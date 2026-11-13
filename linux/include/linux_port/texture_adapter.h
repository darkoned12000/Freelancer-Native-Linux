#pragma once

#if defined(FL_LINUX_32BIT)

#include "linux_port/render_pipeline.h"
#include "render/IRenderPipeline.h"

namespace fl_port {

class TextureAdapter : public IRPTexture {
 public:
  explicit TextureAdapter(RenderPipeline* rp) : rp_(rp) {}

  GENRESULT DACOM_API PrintScreen(void* ParentFileSystem, const char* FileName) override { (void)ParentFileSystem; (void)FileName; return GR_GENERIC; }
  GENRESULT DACOM_API LoadTexture(IFileSystem* FileSystem, const char* FileName, void** OutHandle) override;
  GENRESULT DACOM_API LoadSurfaceData(void* Handle, int Level, unsigned int SrcSize, const void* SrcData) override;
  GENRESULT DACOM_API GetImageInfo(_D3DXIMAGE_INFO* Info, unsigned int SrcSize, const void* SrcData) override;
  GENRESULT DACOM_API LoadCubemap(IFileSystem* FileSystem, const char* FileName, void** OutHandle) override;

  GENRESULT DACOM_API CreateInstance(AGGDESC* Desc, void** Instance) { (void)Desc; (void)Instance; return GR_GENERIC; }
  GENRESULT DACOM_API QueryInterface(const char* iid, void** Out) override { (void)iid; (void)Out; return GR_GENERIC; }
  unsigned long DACOM_API AddRef() override { return 1; }
  unsigned long DACOM_API Release() override { return 1; }

 private:
  RenderPipeline* rp_;
};

}  // namespace fl_port

#endif
