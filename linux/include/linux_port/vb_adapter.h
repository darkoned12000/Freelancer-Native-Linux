#pragma once

// M9.1 step 3: upstream-interface adapter. Implements IRPVertexBuffer,
// IRPIndexBuffer and IVertexBufferManager by delegating to a
// RenderPipeline's VertexBufferManager, so the engine's RP8/RendComp code
// paths can use the native Vulkan buffers transparently.

#if defined(FL_LINUX_32BIT)

#include "linux_port/render_pipeline.h"
#include "render/IRenderPipeline.h"

namespace fl_port {

class VbAdapter : public IRPVertexBuffer, public IRPIndexBuffer, public IVertexBufferManager {
 public:
  explicit VbAdapter(RenderPipeline* rp) : rp_(rp) {}

  // IVertexBufferManager
  GENRESULT DACOM_API VbMgrInit(void* A, void* B, void* C, void* D) override { (void)A;(void)B;(void)C;(void)D; return GR_OK; }
  GENRESULT DACOM_API VbMgrReleaseAll() override { return GR_OK; }
  GENRESULT DACOM_API LockVertexMemory(unsigned int Fvf, unsigned int Count, FVertexMemoryLock* Out) override;
  GENRESULT DACOM_API UnlockVertexMemory(FVertexMemoryLock* Out) override;
  GENRESULT DACOM_API UnsupportedManagerOperation() override { return GR_GENERIC; }
  GENRESULT DACOM_API CopyVertexData(void* Dest, unsigned int Fvf, const FRpVertexSrcDesc* Source,
                                     unsigned int StartVertex, unsigned int Count) override;

  // IRPIndexBuffer
  GENRESULT DACOM_API CreateIb(unsigned int Count, unsigned int** Out, unsigned int Flags) override;
  GENRESULT DACOM_API DestroyIb(void** Handle) override;
  GENRESULT DACOM_API ResizeIb(void* Handle, unsigned int Count) override;
  GENRESULT DACOM_API CopyIndices(void* Handle, unsigned int* OutOffset, const unsigned short* SrcData, unsigned int Count) override;
  GENRESULT DACOM_API LockIb(void* Handle, unsigned int* Offset, void** Out, unsigned int Count) override;
  GENRESULT DACOM_API UnlockIb(void* Handle) override;
  GENRESULT DACOM_API SelectIb(void* Handle, unsigned int BaseVertexIndex, unsigned int Unused3, unsigned int Unused4) override;
  GENRESULT DACOM_API GetIbCapacity(void* Handle, unsigned int* OutCapacity) override;
  GENRESULT DACOM_API IsIbValid(void* Handle) override;

  // IRPVertexBuffer
  GENRESULT DACOM_API CreateVb(unsigned int Fvf, unsigned int Count, unsigned int** Out, unsigned int Flags) override;
  GENRESULT DACOM_API DestroyVb(void** Handle) override;
  GENRESULT DACOM_API ResizeVb(void* Handle, unsigned int Fvf, unsigned int Count) override;
  GENRESULT DACOM_API CopyVertices(void* Handle, unsigned int* OutOffset, const FRpVertexSrcDesc* SrcData, unsigned int StartVertex, unsigned int Count) override;
  GENRESULT DACOM_API LockVb(void* Handle, unsigned int* Offset, void** Out, unsigned int Count) override;
  GENRESULT DACOM_API UnlockVb(void* Handle) override;
  GENRESULT DACOM_API UnsupportedVertexBufferOperation(void* Handle) override { (void)Handle; return static_cast<GENRESULT>(0xFFFFFFF9); }
  GENRESULT DACOM_API SelectVb(void* Handle) override;
  GENRESULT DACOM_API GetVbInfo(void* Handle, unsigned int* OutFvf, unsigned int* OutCount) override;
  GENRESULT DACOM_API IsVbValid(void* Handle) override;

  // IDAComponent
  GENRESULT DACOM_API CreateInstance(AGGDESC* Desc, void** Instance) { (void)Desc; (void)Instance; return GR_GENERIC; }
  GENRESULT DACOM_API QueryInterface(const char* iid, void** Out) override { (void)iid; (void)Out; return GR_GENERIC; }
  unsigned long DACOM_API AddRef() override { return 1; }
  unsigned long DACOM_API Release() override { return 1; }

 private:
  RenderPipeline* rp_;
};

}  // namespace fl_port

#endif  // FL_LINUX_32BIT
