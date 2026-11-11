// M3 harness: x86math (3DMathEngine) numeric/registration checks and
// core config/game data loading via DACOM/ReadFile on Linux-compatible paths.

#include "linux_port/windows.h"
#include "linux_port/path_util.h"
#include "linux_port/platform.h"

#include <dacom.h>
#include <readfile.h>
#include <x86math.h>

#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>

extern "C" int fl_upstream_readfile_integration_register();
extern "C" void fl_upstream_dacom_install_fdump_bridge();
extern "C" void fl_upstream_dacom_register_builtin_components();
extern "C" int fl_upstream_x86math_integration_register();

namespace {

bool g_ok = true;
void ok(const std::string& m){ std::cout << "[ok] " << m << "\n"; }
void fail(const std::string& m){ std::cerr << "[FAIL] " << m << "\n"; g_ok = false; }
void info(const std::string& m){ std::cout << "[info] " << m << "\n"; }

bool nearf(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) <= eps; }

std::string trim_copy(std::string s) {
  size_t a = 0, b = s.size();
  while (a < b && (s[a]==' '||s[a]=='\t'||s[a]=='\r'||s[a]=='\n')) ++a;
  while (b > a && (s[b-1]==' '||s[b-1]=='\t'||s[b-1]=='\r'||s[b-1]=='\n')) --b;
  return s.substr(a,b-a);
}
std::string lower_ascii(std::string s){ for(char&c:s) if(c>='A'&&c<='Z') c=char(c-'A'+'a'); return s; }

std::string get_ini_first_value(const std::filesystem::path& ini_path, const std::string& section_want, const std::string& key_want) {
  std::ifstream in(ini_path);
  if (!in) return {};
  std::string line, current;
  const std::string sec = lower_ascii(section_want);
  while (std::getline(in, line)) {
    auto sc = line.find(';');
    std::string s2 = trim_copy(sc==std::string::npos?line:line.substr(0,sc));
    if (s2.empty()) continue;
    if (s2.front()=='[' && s2.back()==']') { current = lower_ascii(trim_copy(s2.substr(1,s2.size()-2))); continue; }
    if (current != sec) continue;
    auto eq = s2.find('=');
    if (eq == std::string::npos) continue;
    if (lower_ascii(trim_copy(s2.substr(0,eq))) == lower_ascii(key_want)) return trim_copy(s2.substr(eq+1));
  }
  return {};
}

std::filesystem::path make_effective_dacom_ini(const std::filesystem::path& src_ini) {
  std::ifstream in(src_ini);
  if (!in) return {};
  const auto out_path = std::filesystem::temp_directory_path() / ("fl_m3_dacom_init_" + std::to_string(::getpid()) + ".ini");
  std::ofstream out(out_path, std::ios::trunc);
  if (!out) return {};
  std::string line; bool in_lib = false;
  while (std::getline(in, line)) {
    std::string t = trim_copy(line);
    if (!t.empty() && t.front()=='[' && t.back()==']') { in_lib = (lower_ascii(t) == "[libraries]"); out << line << "\n"; continue; }
    if (in_lib) continue;
    out << line << "\n";
  }
  return out.good() ? out_path : std::filesystem::path{};
}

bool read_text_via_win32(const std::filesystem::path& p, std::string& out, size_t max_bytes = 4096) {
  out.clear();
  HANDLE h = CreateFileA(p.string().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (h == INVALID_HANDLE_VALUE) return false;
  std::vector<char> buf(max_bytes);
  DWORD got = 0;
  if (!ReadFile(h, buf.data(), (DWORD)buf.size(), &got, nullptr)) { CloseHandle(h); return false; }
  out.assign(buf.data(), buf.data()+got);
  CloseHandle(h);
  return true;
}

// (IFileSystem file-handle API shape varies; M3 data-loading proof uses
// Win32/shimmed reads and path resolution against DATA/ plus FileSystem instantiation.)

} // namespace

int main(int argc, char** argv) {
  fl_port::log_message(fl_port::LogLevel::Info, "m3_math_data_tests starting");
  std::cout.setf(std::ios::unitbuf);
  fl_upstream_dacom_install_fdump_bridge();

  const std::filesystem::path repo = (argc > 1) ? std::filesystem::path(argv[1]) : std::filesystem::current_path();
  const std::filesystem::path bin_dir = repo / "bin";
  info("repo=" + repo.string());

  std::cout << "\n== DACOM/core init ==\n";
  ICOManager* mgr = DACOM_Acquire();
  if (mgr) ok("DACOM_Acquire"); else fail("DACOM_Acquire");
  if (mgr) { fl_upstream_dacom_register_builtin_components(); ok("DACOM built-ins registered"); }

  auto dacom_ini_resolved = fl_port::resolve_path_case_insensitive(repo, "bin\\DACOM.INI");
  auto freelancer_ini_resolved = fl_port::resolve_path_case_insensitive(repo, "bin\\freelancer.ini");
  if (dacom_ini_resolved) ok("resolve bin\\DACOM.INI"); else fail("resolve bin\\DACOM.INI");
  if (freelancer_ini_resolved) ok("resolve bin\\freelancer.ini"); else fail("resolve bin\\freelancer.ini");

  std::filesystem::path data_root = repo / "DATA";
  if (freelancer_ini_resolved) {
    const std::string dp = get_ini_first_value(*freelancer_ini_resolved, "Freelancer", "data path");
    if (!dp.empty()) {
      if (auto dr = fl_port::resolve_path_case_insensitive(bin_dir, dp)) { data_root = *dr; ok("resolve data path: " + dp); }
      else fail("resolve data path: " + dp);
    } else fail("freelancer.ini missing data path");
  }

  if (mgr && dacom_ini_resolved) {
    auto eff = make_effective_dacom_ini(*dacom_ini_resolved);
    if (!eff.empty()) {
      const GENRESULT gr = mgr->SetINIConfig(eff.string().c_str(), 0);
      if (gr == GR_OK) ok("SetINIConfig loaded (effective config)"); else info("SetINIConfig non-OK (continuing): gr=" + std::to_string((int)gr));
      std::error_code ec; std::filesystem::remove(eff, ec);
    } else fail("prepare effective DACOM init config");
  }

  const int rf_reg = fl_upstream_readfile_integration_register();
  if (rf_reg) ok("ReadFile explicitly registered"); else fail("ReadFile explicit registration");

  IFileSystem* fs = nullptr;
  if (mgr) {
    FFileSystemDesc fsd{};
    fsd.Size = 0x34;
    fsd.ComponentName = "FileSystem";
    fsd.Implementation = "DOS";
    std::string data_path_str = data_root.string();
    fsd.Path = data_path_str.c_str();
    fsd.Access = GENERIC_READ;
    fsd.ShareMode = FILE_SHARE_READ;
    fsd.Creation = OPEN_EXISTING;
    fsd.Flags = 0;
    const GENRESULT gr = mgr->CreateInstance((AGGDESC*)&fsd, (void**)&fs);
    if (gr == GR_OK && fs) ok("CreateInstance(FileSystem/DOS)"); else fail("CreateInstance(FileSystem/DOS) gr=" + std::to_string((int)gr));
  }

  std::cout << "\n== x86math / 3DMathEngine ==\n";
  const int math_reg = fl_upstream_x86math_integration_register();
  if (math_reg) ok("x86math explicitly registered/initialized"); else fail("x86math registration/init");

  I3DMathEngine* math = nullptr;
  if (mgr) {
    FMathDesc md{};
    md.Size = sizeof(FMathDesc);
    md.InterfaceName = "3DMathEngine";
    md.Implementation = "x86";
    const GENRESULT gr = mgr->CreateInstance((AGGDESC*)&md, (void**)&math);
    if (gr == GR_OK && math) ok("CreateInstance(3DMathEngine,x86)"); else fail("CreateInstance(3DMathEngine) gr=" + std::to_string((int)gr));
  }

  if (math) {
    FMatrix I{}; I.D[0][0]=1; I.D[1][1]=1; I.D[2][2]=1;
    FMatrix B{}; B.D[0][0]=2; B.D[1][1]=3; B.D[2][2]=4;
    FMatrix R{};
    if (math->MulMatrix(R, I, B) == GR_OK && nearf(R.D[0][0],2) && nearf(R.D[1][1],3) && nearf(R.D[2][2],4)) ok("MulMatrix identity * diag");
    else fail("MulMatrix identity * diag");

    FVector v{1,2,3}, out{};
    if (math->TransformMatrix(out, B, v) == GR_OK && nearf(out.X,2) && nearf(out.Y,6) && nearf(out.Z,12)) ok("TransformMatrix diag");
    else fail("TransformMatrix diag");

    FTransform xf{}; xf.Rotation = B; xf.Translation = {10,20,30};
    if (math->TransformXform(out, xf, v) == GR_OK && nearf(out.X,12) && nearf(out.Y,26) && nearf(out.Z,42)) ok("TransformXform translate+scale");
    else fail("TransformXform translate+scale");

    FMatrix inv{};
    if (math->Inverse(inv, B) == GR_OK && nearf(inv.D[0][0],0.5f) && nearf(inv.D[1][1],1.0f/3.0f) && nearf(inv.D[2][2],0.25f)) ok("Inverse diagonal");
    else fail("Inverse diagonal");

    FQuaternion q{};
    if (math->MatrixToQuaternion(q, I) == GR_OK) {
      FMatrix back{};
      if (math->QuaternionToMatrix(back, q) == GR_OK && nearf(back.D[0][0],1) && nearf(back.D[1][1],1) && nearf(back.D[2][2],1)) ok("Matrix<->Quaternion identity roundtrip");
      else fail("QuaternionToMatrix identity roundtrip");
    } else fail("MatrixToQuaternion identity");

    FQuaternion qa{1,0,0,0}, qb{1,0,0,0}, qs{};
    if (math->QuatSlerp(qs, qa, qb, 0.5f) == GR_OK && nearf(qs.W,1.0f,1e-3f)) ok("QuatSlerp identity");
    else fail("QuatSlerp identity");

    float sroot=0, invs=0;
    if (math->Sqrt(sroot, 4.0f) == GR_OK && nearf(sroot, 2.0f, 1e-3f)) ok("Sqrt(4)~2"); else fail("Sqrt(4)");
    if (math->InvSqrt(invs, 4.0f) == GR_OK && nearf(invs, 0.5f, 1e-3f)) ok("InvSqrt(4)~0.5"); else fail("InvSqrt(4)");

    // M3 review-fix checks: edge cases, QueryInterface, in-place aliasing.
    {
      float z=123.0f;
      if (math->Sqrt(z, 0.0f) == GR_OK && nearf(z, 0.0f)) ok("Sqrt(0) returns GR_OK/0"); else fail("Sqrt(0)");
      float neg=123.0f;
      if (math->Sqrt(neg, -1.0f) == GR_INVALID_PARMS && nearf(neg, 0.0f)) ok("Sqrt(-1) returns GR_INVALID_PARMS/0"); else fail("Sqrt(-1)");
      float iz=123.0f;
      if (math->InvSqrt(iz, 0.0f) == GR_OK && nearf(iz, 0.0f)) ok("InvSqrt(0) returns GR_OK/0"); else fail("InvSqrt(0)");
      float ineg=123.0f;
      if (math->InvSqrt(ineg, -4.0f) == GR_INVALID_PARMS && nearf(ineg, 0.0f)) ok("InvSqrt(-x) returns GR_INVALID_PARMS/0"); else fail("InvSqrt(-x)");
    }
    {
      void* qi = nullptr;
      if (math->QueryInterface("I3DMathEngine", &qi) == GR_OK && qi == static_cast<void*>(math)) ok("QueryInterface(I3DMathEngine)"); else fail("QueryInterface(I3DMathEngine)");
      qi = nullptr;
      if (math->QueryInterface("IComponentFactory", &qi) == GR_OK && qi != nullptr) ok("QueryInterface(IComponentFactory)"); else fail("QueryInterface(IComponentFactory)");
      qi = (void*)0x1;
      if (math->QueryInterface("NoSuchInterface", &qi) == GR_INTERFACE_UNSUPPORTED && qi == nullptr) ok("QueryInterface unknown -> GR_INTERFACE_UNSUPPORTED/null"); else fail("QueryInterface unknown");
    }
    {
      FVector va{1,2,3};
      if (math->TransformMatrix(va, B, va) == GR_OK && nearf(va.X,2) && nearf(va.Y,6) && nearf(va.Z,12)) ok("aliasing: TransformMatrix(dst==src)"); else fail("aliasing: TransformMatrix(dst==src)");
      FVector vx{1,2,3};
      if (math->TransformXform(vx, xf, vx) == GR_OK && nearf(vx.X,12) && nearf(vx.Y,26) && nearf(vx.Z,42)) ok("aliasing: TransformXform(dst==src)"); else fail("aliasing: TransformXform(dst==src)");
      FVector vt{1,2,3}; // B^T*vt = {2,6,12}
      if (math->TransposeTransform(vt, B, vt) == GR_OK && nearf(vt.X,2) && nearf(vt.Y,6) && nearf(vt.Z,12)) ok("aliasing: TransposeTransform(dst==src)"); else fail("aliasing: TransposeTransform(dst==src)");
    }
    {
      FMatrix A2{}; A2.D[0][0]=2; A2.D[1][1]=3; A2.D[2][2]=4; // same as B
      FMatrix D = A2;
      if (math->MulMatrix(D, A2, D) == GR_OK && nearf(D.D[0][0],4) && nearf(D.D[1][1],9) && nearf(D.D[2][2],16)) ok("aliasing: MulMatrix(dst==B)"); else fail("aliasing: MulMatrix(dst==B)");
      D = A2;
      if (math->MulMatrix(D, D, A2) == GR_OK && nearf(D.D[0][0],4) && nearf(D.D[1][1],9) && nearf(D.D[2][2],16)) ok("aliasing: MulMatrix(dst==A)"); else fail("aliasing: MulMatrix(dst==A)");
    }
    {
      FMatrix M = B;
      if (math->Inverse(M, M) == GR_OK && nearf(M.D[0][0],0.5f) && nearf(M.D[1][1],1.0f/3.0f) && nearf(M.D[2][2],0.25f)) ok("aliasing: Inverse(dst==src)"); else fail("aliasing: Inverse(dst==src)");
    }
    {
      FQuaternion qa2{1,0,0,0}, qb2{0,1,0,0};
      FQuaternion a = qa2, b = qb2, ref{};
      math->MulQuat(ref, qa2, qb2); // (1,0,0,0)*(0,1,0,0) in (w,x,y,z) -> (0,1,0,0)
      FQuaternion mq = a;
      if (math->MulQuat(mq, mq, b) == GR_OK && nearf(mq.W,ref.W) && nearf(mq.X,ref.X) && nearf(mq.Y,ref.Y) && nearf(mq.Z,ref.Z)) ok("aliasing: MulQuat(dst==A)"); else fail("aliasing: MulQuat(dst==A)");
      mq = b;
      if (math->MulQuat(mq, a, mq) == GR_OK && nearf(mq.W,ref.W) && nearf(mq.X,ref.X) && nearf(mq.Y,ref.Y) && nearf(mq.Z,ref.Z)) ok("aliasing: MulQuat(dst==B)"); else fail("aliasing: MulQuat(dst==B)");
    }
    {
      FTransform ta{}; ta.Rotation = I; ta.Translation = {1,2,3};
      FTransform tb{}; tb.Rotation = B; tb.Translation = {4,5,6};
      FTransform tref{}; math->MulTransform(tref, ta, tb);
      FTransform mt = ta;
      if (math->MulTransform(mt, mt, tb) == GR_OK && nearf(mt.Translation.X,tref.Translation.X) && nearf(mt.Translation.Y,tref.Translation.Y) && nearf(mt.Translation.Z,tref.Translation.Z) && nearf(mt.Rotation.D[0][0],tref.Rotation.D[0][0]) && nearf(mt.Rotation.D[1][1],tref.Rotation.D[1][1]) && nearf(mt.Rotation.D[2][2],tref.Rotation.D[2][2])) ok("aliasing: MulTransform(dst==A)"); else fail("aliasing: MulTransform(dst==A)");
      mt = tb;
      if (math->MulTransform(mt, ta, mt) == GR_OK && nearf(mt.Translation.X,tref.Translation.X) && nearf(mt.Translation.Y,tref.Translation.Y) && nearf(mt.Translation.Z,tref.Translation.Z) && nearf(mt.Rotation.D[0][0],tref.Rotation.D[0][0]) && nearf(mt.Rotation.D[1][1],tref.Rotation.D[1][1]) && nearf(mt.Rotation.D[2][2],tref.Rotation.D[2][2])) ok("aliasing: MulTransform(dst==B)"); else fail("aliasing: MulTransform(dst==B)");
    }
    {
      FTransform tx{}; tx.Rotation = B; tx.Translation = {10,20,30};
      FTransform tinv{}; float w = 0;
      const GENRESULT gr0 = math->GeneralInverse(tinv, w, tx);
      FTransform txa = tx; float wa = 0;
      const GENRESULT gr1 = math->GeneralInverse(txa, wa, txa);
      if (gr1 == gr0 && nearf(wa, w, 1e-3f) && nearf(txa.Rotation.D[0][0], tinv.Rotation.D[0][0]) && nearf(txa.Rotation.D[1][1], tinv.Rotation.D[1][1]) && nearf(txa.Rotation.D[2][2], tinv.Rotation.D[2][2]) && nearf(txa.Translation.X, tinv.Translation.X) && nearf(txa.Translation.Y, tinv.Translation.Y) && nearf(txa.Translation.Z, tinv.Translation.Z)) ok("aliasing: GeneralInverse(dst==src)"); else fail("aliasing: GeneralInverse(dst==src)");
    }

    math->Release();
  }

  std::cout << "\n== Path resolution + core config/data loading ==\n";

  auto solar = fl_port::resolve_path_case_insensitive(repo, "DATA\\solar\\solararch.ini");
  if (solar) ok("resolve DATA\\solar\\solararch.ini (case-insensitive/backslashes)"); else fail("resolve DATA\\solar\\solararch.ini");
  auto uni = fl_port::resolve_path_case_insensitive(repo, "DATA\\UNIVERSE\\universe.ini");
  if (uni) ok("resolve DATA\\UNIVERSE\\universe.ini"); else fail("resolve DATA\\UNIVERSE\\universe.ini");

  if (dacom_ini_resolved) {
    std::string t; if (read_text_via_win32(*dacom_ini_resolved, t, 128)) ok("Win32 read bin/DACOM.INI"); else fail("Win32 read bin/DACOM.INI");
  }
  if (freelancer_ini_resolved) {
    std::string t; if (read_text_via_win32(*freelancer_ini_resolved, t, 256) && lower_ascii(t).find("[freelancer]") != std::string::npos) ok("Win32 read+spot-check freelancer.ini [Freelancer]"); else fail("Win32 read/spot-check freelancer.ini");
  }

  if (fs) {
    ok("FileSystem(DOS) core component instantiated for DATA root");
  }

  // Load/read core game data from DATA/ via Linux-compatible path resolution + Win32 shim APIs.
  // Some Freelancer game data files are compiled BINI (binary) rather than text INI.
  if (solar) {
    std::string t;
    if (read_text_via_win32(*solar, t, 512) && !t.empty()) ok("read DATA\\solar\\solararch.ini (resolved, case-insensitive)");
    else fail("read resolved DATA\\solar\\solararch.ini");
    if (t.size() >= 4 && t.rfind("BINI", 0) == 0) ok("spot-check solararch.ini: BINI signature/content");
    else if (!t.empty() && (t.find('[') != std::string::npos || t.find(';') != std::string::npos)) ok("spot-check solararch.ini: text INI content");
    else { info("spot-check debug solararch.ini size=" + std::to_string(t.size()) + " prefix_hex=" + (t.size()>=4 ? std::to_string((unsigned char)t[0])+","+std::to_string((unsigned char)t[1])+","+std::to_string((unsigned char)t[2])+","+std::to_string((unsigned char)t[3]) : "short")); fail("spot-check solararch.ini content"); }
  }
  if (uni) {
    std::string t;
    if (read_text_via_win32(*uni, t, 512) && !t.empty()) ok("read DATA\\UNIVERSE\\universe.ini (resolved)");
    else fail("read resolved DATA\\UNIVERSE\\universe.ini");
    if (t.size() >= 4 && t.rfind("BINI", 0) == 0) ok("spot-check universe.ini: BINI signature/content");
    else if (lower_ascii(t).find("[universe]") != std::string::npos || t.find('[') != std::string::npos) ok("spot-check universe.ini: text INI content");
    else { info("spot-check debug universe.ini size=" + std::to_string(t.size()) + " prefix_hex=" + (t.size()>=4 ? std::to_string((unsigned char)t[0])+","+std::to_string((unsigned char)t[1])+","+std::to_string((unsigned char)t[2])+","+std::to_string((unsigned char)t[3]) : "short")); fail("spot-check universe.ini content"); }
  }

  // Behavior/consistency checks: resolved config/data paths map to real files on case-sensitive FS
  if (solar && std::filesystem::exists(*solar)) ok("consistency: resolved solararch.ini exists on disk");
  else if (solar) fail("consistency: resolved solararch.ini missing on disk");
  if (uni && std::filesystem::exists(*uni)) ok("consistency: resolved universe.ini exists on disk");
  else if (uni) fail("consistency: resolved universe.ini missing on disk");

  std::string ftxt;
  if (freelancer_ini_resolved && read_text_via_win32(*freelancer_ini_resolved, ftxt, 4096)) {
    if (ftxt.find("data path") != std::string::npos) ok("config consistency: freelancer.ini contains 'data path'");
    else fail("config consistency: freelancer.ini missing 'data path'");
    if (ftxt.find("LOCAL_SERVER") != std::string::npos) ok("config consistency: freelancer.ini references LOCAL_SERVER dll entry");
    else info("config consistency: LOCAL_SERVER entry not found in prefix (informational)");
  }

  if (!fl_port::resolve_path_case_insensitive(repo, "DATA\\no_such_dir_12345\\missing.ini")) ok("missing path resolution returns failure"); else fail("missing path resolution should fail");

  if (fs) fs->Release();

  fl_port::log_message(fl_port::LogLevel::Info, "m3_math_data_tests complete");
  std::cout << "\nm3_math_data_tests: " << (g_ok ? "PASS" : "FAIL") << "\n";
  return g_ok ? 0 : 1;
}
