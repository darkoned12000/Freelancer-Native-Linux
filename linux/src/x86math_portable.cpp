// Portable Linux bring-up implementation of x86math (3DMathEngine) for M3.
// Preserves DACOM interface/registration semantics and core math behavior;
// MSVC/x87-specific instruction-level/numeric edge behavior may differ.

#if !defined(_WIN32)
#include "linux_port/windows.h"
#include "linux_port/platform.h"
#endif

#include "x86math.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>

static int CallDacomStricmp(const char* Left, const char* Right) {
  return _stricmp(Left ? Left : "", Right ? Right : "");
}

namespace {
const char G_ComponentName[] = "3DMathEngine";
const char* G_ImplementationId = "x86";
const float G_MinDet = 1e-8f;
const unsigned int G_RegisterPriority = 0x40000000u;
const unsigned int G_ComponentNameBytes = 13;
static_assert(sizeof(G_ComponentName) == G_ComponentNameBytes, "CMPSB ECX=0xD");

unsigned char G_InvSqrtTable[0x400];

void BuildInvSqrtTable() {
  for (unsigned int Index = 0; Index < 0x400; ++Index) {
    unsigned int Bits = (Index | 0xFC00u) << 14;
    union { unsigned int AsBits; float Value; } Pun{};
    Pun.AsBits = Bits;
    Pun.Value = static_cast<float>(1.0 / std::sqrt(static_cast<double>(Pun.Value)));
    unsigned int RecipBits = Pun.AsBits;
    G_InvSqrtTable[Index] = static_cast<unsigned char>((RecipBits + 0x2000u) >> 15);
  }
  G_InvSqrtTable[0x200] = 0xFF;
}

float TableInvSqrt(float X) {
  union { unsigned int Bits; float Value; } Pun{};
  Pun.Value = X;
  unsigned int Exponent = (Pun.Bits >> 23) & 0xFFu;
  const unsigned int TableIndex = (Pun.Bits >> 14) & 0x3FFu;
  unsigned int Guess = (0x5F000000u - (Exponent << 22)) & 0xFF800000u;
  Guess |= static_cast<unsigned int>(G_InvSqrtTable[TableIndex]) << 15;
  Pun.Bits = Guess;
  float Y = Pun.Value;
  Y = (3.0f - Y * Y * X) * Y * 0.5f;
  Y = (3.0f - Y * Y * X) * Y * 0.5f;
  return Y;
}

// Alias-safe helpers: compute via temporaries/buffers so &Dst may alias sources.
void MulVecByMatrix(FVector& Dst, const FMatrix& Matrix, const FVector& Source) {
  const float x = Matrix.D[0][0]*Source.X + Matrix.D[0][1]*Source.Y + Matrix.D[0][2]*Source.Z;
  const float y = Matrix.D[1][0]*Source.X + Matrix.D[1][1]*Source.Y + Matrix.D[1][2]*Source.Z;
  const float z = Matrix.D[2][0]*Source.X + Matrix.D[2][1]*Source.Y + Matrix.D[2][2]*Source.Z;
  Dst.X = x; Dst.Y = y; Dst.Z = z;
}
void MulVecByTranspose(FVector& Dst, const FMatrix& Matrix, const FVector& Source) {
  const float x = Matrix.D[0][0]*Source.X + Matrix.D[1][0]*Source.Y + Matrix.D[2][0]*Source.Z;
  const float y = Matrix.D[0][1]*Source.X + Matrix.D[1][1]*Source.Y + Matrix.D[2][1]*Source.Z;
  const float z = Matrix.D[0][2]*Source.X + Matrix.D[1][2]*Source.Y + Matrix.D[2][2]*Source.Z;
  Dst.X = x; Dst.Y = y; Dst.Z = z;
}
void TransposeMatrix(FMatrix& Dst, const FMatrix& Source) {
  FMatrix tmp{};
  tmp.D[0][0]=Source.D[0][0]; tmp.D[0][1]=Source.D[1][0]; tmp.D[0][2]=Source.D[2][0];
  tmp.D[1][0]=Source.D[0][1]; tmp.D[1][1]=Source.D[1][1]; tmp.D[1][2]=Source.D[2][1];
  tmp.D[2][0]=Source.D[0][2]; tmp.D[2][1]=Source.D[1][2]; tmp.D[2][2]=Source.D[2][2];
  Dst = tmp;
}
void WriteAdjugate(FMatrix& Dst, const FMatrix& S, float Scale) {
  FMatrix tmp{};
  tmp.D[0][0] = (S.D[1][1]*S.D[2][2] - S.D[1][2]*S.D[2][1]) * Scale;
  tmp.D[1][0] = -(S.D[1][0]*S.D[2][2] - S.D[1][2]*S.D[2][0]) * Scale;
  tmp.D[2][0] = (S.D[1][0]*S.D[2][1] - S.D[1][1]*S.D[2][0]) * Scale;
  tmp.D[0][1] = -(S.D[0][1]*S.D[2][2] - S.D[0][2]*S.D[2][1]) * Scale;
  tmp.D[1][1] = (S.D[0][0]*S.D[2][2] - S.D[0][2]*S.D[2][0]) * Scale;
  tmp.D[2][1] = -(S.D[0][0]*S.D[2][1] - S.D[0][1]*S.D[2][0]) * Scale;
  tmp.D[0][2] = (S.D[0][1]*S.D[1][2] - S.D[0][2]*S.D[1][1]) * Scale;
  tmp.D[1][2] = -(S.D[0][0]*S.D[1][2] - S.D[0][2]*S.D[1][0]) * Scale;
  tmp.D[2][2] = (S.D[0][0]*S.D[1][1] - S.D[0][1]*S.D[1][0]) * Scale;
  Dst = tmp;
}

struct FMathEngine final : I3DMathEngine, IComponentFactory {
  GENRESULT DACOM_API QueryInterface(const char* iid, void** Out) override {
    if (!Out) return GR_INVALID_PARMS;
    if (!iid || std::strcmp(iid, "I3DMathEngine") == 0 || std::strcmp(iid, G_ComponentName) == 0) {
      *Out = static_cast<I3DMathEngine*>(this);
      AddRef();
      return GR_OK;
    }
    if (std::strcmp(iid, "IComponentFactory") == 0) {
      *Out = static_cast<IComponentFactory*>(this);
      AddRef();
      return GR_OK;
    }
    *Out = nullptr;
    return GR_INTERFACE_UNSUPPORTED;
  }
  unsigned long DACOM_API AddRef() override { return 1; }
  unsigned long DACOM_API Release() override { return 1; }

  GENRESULT DACOM_API CreateInstance(AGGDESC* Desc, void** Instance) override {
    if (!Instance) return GR_INVALID_PARMS;
    *Instance = nullptr;
    FMathDesc* Info = reinterpret_cast<FMathDesc*>(Desc);
    if (!Info || !Info->InterfaceName) return GR_INVALID_PARMS;
    if (Info->Size != sizeof(FMathDesc) || std::memcmp(Info->InterfaceName, G_ComponentName, G_ComponentNameBytes) != 0)
      return GR_INTERFACE_UNSUPPORTED;
    if (Info->Implementation && CallDacomStricmp(Info->Implementation, G_ImplementationId) != 0)
      return GR_INTERFACE_UNSUPPORTED;
    AddRef();
    *Instance = static_cast<I3DMathEngine*>(this);
    return GR_OK;
  }

  GENRESULT DACOM_API Inverse(FMatrix& Dst, const FMatrix& Source) override {
    const FMatrix src = Source; // alias-safe
    const float det = Det(src);
    FMatrix out{};
    if (std::fabs(det) > G_MinDet) { WriteAdjugate(out, src, 1.0f / det); Dst = out; return GR_OK; }
    WriteAdjugate(out, src, 1.0f);
    Dst = out;
    return GR_INVALID_PARMS;
  }
  GENRESULT DACOM_API Scale(FMatrix& Dst, const FMatrix& Source, float S) override {
    for(int r=0;r<3;++r) for(int c=0;c<3;++c) Dst.D[r][c]=Source.D[r][c]*S; return GR_OK;
  }
  float DACOM_API Det(const FMatrix& S) override {
    return S.D[0][0]*S.D[1][1]*S.D[2][2] + S.D[0][1]*S.D[1][2]*S.D[2][0] + S.D[0][2]*S.D[1][0]*S.D[2][1]
         - S.D[0][0]*S.D[1][2]*S.D[2][1] - S.D[0][1]*S.D[1][0]*S.D[2][2] - S.D[0][2]*S.D[1][1]*S.D[2][0];
  }
  GENRESULT DACOM_API MulQuat(FQuaternion& Dst, const FQuaternion& A, const FQuaternion& B) override {
    const FQuaternion out{
      A.W*B.W - (A.X*B.X + A.Y*B.Y + A.Z*B.Z),
      A.W*B.X + B.W*A.X + (A.Y*B.Z - A.Z*B.Y),
      A.W*B.Y + B.W*A.Y + (A.Z*B.X - A.X*B.Z),
      A.W*B.Z + B.W*A.Z + (A.X*B.Y - A.Y*B.X)
    };
    Dst = out;
    return GR_OK;
  }
  GENRESULT DACOM_API MulTransform(FTransform& Dst, const FTransform& A, const FTransform& B) override {
    FTransform out{};
    out.Rotation.D[0][0]=A.Rotation.D[0][0]*B.Rotation.D[0][0]+A.Rotation.D[0][1]*B.Rotation.D[1][0]+A.Rotation.D[0][2]*B.Rotation.D[2][0];
    out.Rotation.D[0][1]=A.Rotation.D[0][0]*B.Rotation.D[0][1]+A.Rotation.D[0][1]*B.Rotation.D[1][1]+A.Rotation.D[0][2]*B.Rotation.D[2][1];
    out.Rotation.D[0][2]=A.Rotation.D[0][0]*B.Rotation.D[0][2]+A.Rotation.D[0][1]*B.Rotation.D[1][2]+A.Rotation.D[0][2]*B.Rotation.D[2][2];
    out.Translation.X   =A.Rotation.D[0][0]*B.Translation.X   +A.Rotation.D[0][1]*B.Translation.Y   +A.Rotation.D[0][2]*B.Translation.Z   +A.Translation.X;
    out.Rotation.D[1][0]=A.Rotation.D[1][0]*B.Rotation.D[0][0]+A.Rotation.D[1][1]*B.Rotation.D[1][0]+A.Rotation.D[1][2]*B.Rotation.D[2][0];
    out.Rotation.D[1][1]=A.Rotation.D[1][0]*B.Rotation.D[0][1]+A.Rotation.D[1][1]*B.Rotation.D[1][1]+A.Rotation.D[1][2]*B.Rotation.D[2][1];
    out.Rotation.D[1][2]=A.Rotation.D[1][0]*B.Rotation.D[0][2]+A.Rotation.D[1][1]*B.Rotation.D[1][2]+A.Rotation.D[1][2]*B.Rotation.D[2][2];
    out.Translation.Y   =A.Rotation.D[1][0]*B.Translation.X   +A.Rotation.D[1][1]*B.Translation.Y   +A.Rotation.D[1][2]*B.Translation.Z   +A.Translation.Y;
    out.Rotation.D[2][0]=A.Rotation.D[2][0]*B.Rotation.D[0][0]+A.Rotation.D[2][1]*B.Rotation.D[1][0]+A.Rotation.D[2][2]*B.Rotation.D[2][0];
    out.Rotation.D[2][1]=A.Rotation.D[2][0]*B.Rotation.D[0][1]+A.Rotation.D[2][1]*B.Rotation.D[1][1]+A.Rotation.D[2][2]*B.Rotation.D[2][1];
    out.Rotation.D[2][2]=A.Rotation.D[2][0]*B.Rotation.D[0][2]+A.Rotation.D[2][1]*B.Rotation.D[1][2]+A.Rotation.D[2][2]*B.Rotation.D[2][2];
    out.Translation.Z   =A.Rotation.D[2][0]*B.Translation.X   +A.Rotation.D[2][1]*B.Translation.Y   +A.Rotation.D[2][2]*B.Translation.Z   +A.Translation.Z;
    Dst = out;
    return GR_OK;
  }
  GENRESULT DACOM_API MulMatrix(FMatrix& Dst, const FMatrix& A, const FMatrix& B) override {
    FMatrix out{};
    for (int r=0;r<3;++r){
      out.D[r][0]=A.D[r][0]*B.D[0][0]+A.D[r][1]*B.D[1][0]+A.D[r][2]*B.D[2][0];
      out.D[r][1]=A.D[r][0]*B.D[0][1]+A.D[r][1]*B.D[1][1]+A.D[r][2]*B.D[2][1];
      out.D[r][2]=A.D[r][0]*B.D[0][2]+A.D[r][1]*B.D[1][2]+A.D[r][2]*B.D[2][2];
    }
    Dst = out;
    return GR_OK;
  }
  GENRESULT DACOM_API TransformQuat(FVector& Dst, const FQuaternion& Quat, const FVector& Source) override {
    const float dotVv = Quat.X*Quat.X + Quat.Y*Quat.Y + Quat.Z*Quat.Z;
    const float scale = Quat.W*Quat.W - dotVv;
    const float dotQv = Quat.X*Source.X + Quat.Y*Source.Y + Quat.Z*Source.Z;
    const float cx = Quat.Y*Source.Z - Quat.Z*Source.Y;
    const float cy = Quat.Z*Source.X - Quat.X*Source.Z;
    const float cz = Quat.X*Source.Y - Quat.Y*Source.X;
    const FVector out{
      Source.X*scale + Quat.X*(2.0f*dotQv) + cx*(Quat.W*2.0f),
      Source.Y*scale + Quat.Y*(2.0f*dotQv) + cy*(Quat.W*2.0f),
      Source.Z*scale + Quat.Z*(2.0f*dotQv) + cz*(Quat.W*2.0f)
    };
    Dst = out;
    return GR_OK;
  }
  GENRESULT DACOM_API TransformXform(FVector& Dst, const FTransform& Xform, const FVector& Source) override {
    const FVector out{
      Xform.Rotation.D[0][0]*Source.X + Xform.Rotation.D[0][1]*Source.Y + Xform.Rotation.D[0][2]*Source.Z + Xform.Translation.X,
      Xform.Rotation.D[1][0]*Source.X + Xform.Rotation.D[1][1]*Source.Y + Xform.Rotation.D[1][2]*Source.Z + Xform.Translation.Y,
      Xform.Rotation.D[2][0]*Source.X + Xform.Rotation.D[2][1]*Source.Y + Xform.Rotation.D[2][2]*Source.Z + Xform.Translation.Z
    };
    Dst = out;
    return GR_OK;
  }
  GENRESULT DACOM_API TransformMatrix(FVector& Dst, const FMatrix& Matrix, const FVector& Source) override { MulVecByMatrix(Dst, Matrix, Source); return GR_OK; }
  GENRESULT DACOM_API Rotate(FVector& Dst, const FTransform& Xform, const FVector& Source) override { MulVecByMatrix(Dst, Xform.Rotation, Source); return GR_OK; }
  GENRESULT DACOM_API InverseRotate(FVector& Dst, const FTransform& Xform, const FVector& Source) override { MulVecByTranspose(Dst, Xform.Rotation, Source); return GR_OK; }
  GENRESULT DACOM_API TransposeTransform(FVector& Dst, const FMatrix& Matrix, const FVector& Source) override { MulVecByTranspose(Dst, Matrix, Source); return GR_OK; }
  GENRESULT DACOM_API InverseTransform(FVector& Dst, const FTransform& Xform, const FVector& Source) override {
    const FVector delta{Source.X - Xform.Translation.X, Source.Y - Xform.Translation.Y, Source.Z - Xform.Translation.Z};
    MulVecByTranspose(Dst, Xform.Rotation, delta); return GR_OK;
  }
  GENRESULT DACOM_API TransformListXform(FVector* Dst, const FTransform& Xform, const FVector* Source, int Count) override {
    for (int i=0;i<Count;++i) TransformXform(Dst[i], Xform, Source[i]); return GR_OK;
  }
  GENRESULT DACOM_API TransformListMatrix(FVector* Dst, const FMatrix& Matrix, const FVector* Source, int Count) override {
    for (int i=0;i<Count;++i) MulVecByMatrix(Dst[i], Matrix, Source[i]); return GR_OK;
  }
  GENRESULT DACOM_API TransposeTransformList(FVector* Dst, const FMatrix& Matrix, const FVector* Source, int Count) override {
    FMatrix T; TransposeMatrix(T, Matrix); for(int i=0;i<Count;++i) MulVecByMatrix(Dst[i], T, Source[i]); return GR_OK;
  }
  GENRESULT DACOM_API InverseTransformList(FVector* Dst, const FTransform& Xform, const FVector* Source, int Count) override {
    for(int i=0;i<Count;++i) InverseTransform(Dst[i], Xform, Source[i]); return GR_OK;
  }
  GENRESULT DACOM_API MatrixToQuaternion(FQuaternion& Dst, const FMatrix& Source) override {
    const float trace = Source.D[0][0] + Source.D[1][1] + Source.D[2][2];
    if (trace > 0.0f) {
      const float s = std::sqrt(trace + 1.0f) * 2.0f;
      Dst.W = 0.25f*s; Dst.X=(Source.D[2][1]-Source.D[1][2])/s; Dst.Y=(Source.D[0][2]-Source.D[2][0])/s; Dst.Z=(Source.D[1][0]-Source.D[0][1])/s;
    } else if (Source.D[0][0] > Source.D[1][1] && Source.D[0][0] > Source.D[2][2]) {
      const float s = std::sqrt(1.0f + Source.D[0][0] - Source.D[1][1] - Source.D[2][2]) * 2.0f;
      Dst.W=(Source.D[2][1]-Source.D[1][2])/s; Dst.X=0.25f*s; Dst.Y=(Source.D[0][1]+Source.D[1][0])/s; Dst.Z=(Source.D[0][2]+Source.D[2][0])/s;
    } else if (Source.D[1][1] > Source.D[2][2]) {
      const float s = std::sqrt(1.0f + Source.D[1][1] - Source.D[0][0] - Source.D[2][2]) * 2.0f;
      Dst.W=(Source.D[0][2]-Source.D[2][0])/s; Dst.X=(Source.D[0][1]+Source.D[1][0])/s; Dst.Y=0.25f*s; Dst.Z=(Source.D[1][2]+Source.D[2][1])/s;
    } else {
      const float s = std::sqrt(1.0f + Source.D[2][2] - Source.D[0][0] - Source.D[1][1]) * 2.0f;
      Dst.W=(Source.D[1][0]-Source.D[0][1])/s; Dst.X=(Source.D[0][2]+Source.D[2][0])/s; Dst.Y=(Source.D[1][2]+Source.D[2][1])/s; Dst.Z=0.25f*s;
    }
    return GR_OK;
  }
  GENRESULT DACOM_API QuaternionToMatrix(FMatrix& Dst, const FQuaternion& Quat) override {
    const float x=Quat.X,y=Quat.Y,z=Quat.Z,w=Quat.W;
    const float xx=x*x,yy=y*y,zz=z*z,xy=x*y,xz=x*z,yz=y*z,wx=w*x,wy=w*y,wz=w*z;
    Dst.D[0][0]=1.0f-2.0f*(yy+zz); Dst.D[0][1]=2.0f*(xy-wz); Dst.D[0][2]=2.0f*(xz+wy);
    Dst.D[1][0]=2.0f*(xy+wz);      Dst.D[1][1]=1.0f-2.0f*(xx+zz); Dst.D[1][2]=2.0f*(yz-wx);
    Dst.D[2][0]=2.0f*(xz-wy);      Dst.D[2][1]=2.0f*(yz+wx);      Dst.D[2][2]=1.0f-2.0f*(xx+yy);
    return GR_OK;
  }
  GENRESULT DACOM_API QuatSlerp(FQuaternion& Dst, const FQuaternion& A, const FQuaternion& B, float T) override { return QuatSlerpValue(Dst,A,B,&T); }
  GENRESULT DACOM_API QuatSlerpValue(FQuaternion& Dst, const FQuaternion& A, const FQuaternion& B, const float* T) override {
    // Intentional NLERP (normalized linear interpolation) approximation for bring-up:
    // shortest-arc lerp followed by normalization; not true constant-angular-velocity SLERP.
    const float t = *T;
    const float dot = A.X*B.X + A.Y*B.Y + A.Z*B.Z + A.W*B.W;
    FQuaternion tmp;
    if (dot < 0.0f) { tmp.W=A.W+(-B.W-A.W)*t; tmp.X=A.X+(-B.X-A.X)*t; tmp.Y=A.Y+(-B.Y-A.Y)*t; tmp.Z=A.Z+(-B.Z-A.Z)*t; }
    else { tmp.W=A.W+(B.W-A.W)*t; tmp.X=A.X+(B.X-A.X)*t; tmp.Y=A.Y+(B.Y-A.Y)*t; tmp.Z=A.Z+(B.Z-A.Z)*t; }
    const float len = std::sqrt(tmp.W*tmp.W + tmp.X*tmp.X + tmp.Y*tmp.Y + tmp.Z*tmp.Z);
    if (len > 0.0f) { const float inv=1.0f/len; Dst = FQuaternion{tmp.W*inv, tmp.X*inv, tmp.Y*inv, tmp.Z*inv}; }
    else Dst = A;
    return GR_OK;
  }
  GENRESULT DACOM_API GeneralInverse(FTransform& Dst, float& W, const FTransform& Source) override {
    const FTransform src = Source; // alias-safe (Dst may alias Source)
    const float det = Det(src.Rotation);
    GENRESULT res = GR_OK; float invDet = 1.0f;
    if (std::fabs(det) > G_MinDet) invDet = 1.0f/det; else res = GR_INVALID_PARMS;
    FTransform out{};
    WriteAdjugate(out.Rotation, src.Rotation, invDet);
    const FMatrix& M = src.Rotation; const FVector& T = src.Translation;
    out.Translation.X = -(M.D[0][1]*M.D[1][2]*T.Z + M.D[1][1]*M.D[2][2]*T.X + M.D[2][1]*M.D[0][2]*T.Y - M.D[0][1]*M.D[2][2]*T.Y - M.D[1][1]*M.D[0][2]*T.Z - M.D[2][1]*M.D[1][2]*T.X) * invDet;
    out.Translation.Y =  (M.D[0][0]*M.D[1][2]*T.Z + M.D[1][0]*M.D[2][2]*T.X + M.D[2][0]*M.D[0][2]*T.Y - M.D[0][0]*M.D[2][2]*T.Y - M.D[1][0]*M.D[0][2]*T.Z - M.D[2][0]*M.D[1][2]*T.X) * invDet;
    out.Translation.Z = -(M.D[0][0]*M.D[1][1]*T.Z + M.D[1][0]*M.D[2][1]*T.X + M.D[2][0]*M.D[0][1]*T.Y - M.D[0][0]*M.D[2][1]*T.Y - M.D[1][0]*M.D[0][1]*T.Z - M.D[2][0]*M.D[1][1]*T.X) * invDet;
    W = (M.D[0][0]*M.D[1][1]*M.D[2][2] + M.D[1][0]*M.D[2][1]*M.D[0][2] + M.D[2][0]*M.D[0][1]*M.D[1][2] - M.D[2][0]*M.D[1][1]*M.D[0][2] - M.D[0][0]*M.D[2][1]*M.D[1][2] - M.D[1][0]*M.D[0][1]*M.D[2][2]) * invDet;
    Dst = out;
    return res;
  }
  GENRESULT DACOM_API InvSqrt(float& Dst, float X) override {
    if (X <= 0.0f) { Dst = 0.0f; return (X == 0.0f) ? GR_OK : GR_INVALID_PARMS; }
    Dst = TableInvSqrt(X);
    return GR_OK;
  }
  GENRESULT DACOM_API Sqrt(float& Dst, float X) override {
    if (X < 0.0f) { Dst = 0.0f; return GR_INVALID_PARMS; }
    if (X == 0.0f) { Dst = 0.0f; return GR_OK; }
    Dst = TableInvSqrt(X) * X;
    return GR_OK;
  }
};

static_assert(sizeof(FMathEngine) == 2 * sizeof(void*), "engine must contain exactly two vptrs");
FMathEngine* G_Engine = nullptr;

void fl_x86math_initialize_engine() {
  if (G_Engine) return;
  void* mem = ::operator new(sizeof(FMathEngine), std::nothrow);
  if (!mem) { G_Engine = nullptr; return; }
  G_Engine = new (mem) FMathEngine();
  BuildInvSqrtTable();
  if (ICOManager* dacom = DACOM_Acquire()) {
    dacom->RegisterComponent(static_cast<IComponentFactory*>(G_Engine), G_ComponentName,
                             reinterpret_cast<void*>(static_cast<uintptr_t>(G_RegisterPriority)));
  }
}
} // namespace

#if defined(__GNUC__) || defined(__clang__)
#define FL_UPSTREAM_EXPORT __attribute__((visibility("default")))
#else
#define FL_UPSTREAM_EXPORT
#endif
extern "C" FL_UPSTREAM_EXPORT int fl_upstream_x86math_integration_register() {
  fl_x86math_initialize_engine();
  return G_Engine ? 1 : 0;
}
