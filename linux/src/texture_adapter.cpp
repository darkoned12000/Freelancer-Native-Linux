#if defined(FL_LINUX_32BIT)

#include "linux_port/texture_adapter.h"

namespace fl_port {

GENRESULT TextureAdapter::LoadTexture(IFileSystem* /*FileSystem*/, const char* FileName,
                                     void** OutHandle) {
  if (!rp_ || !FileName || !OutHandle) return GR_INVALID_PARMS;
  return rp_->textures().load_texture(FileName, OutHandle) == 0 ? GR_OK : GR_GENERIC;
}

GENRESULT TextureAdapter::LoadSurfaceData(void* Handle, int Level, unsigned int SrcSize,
                                         const void* SrcData) {
  if (!rp_) return GR_GENERIC;
  unsigned int fmt = 0;
  return rp_->textures().set_texture_level_data(Handle, Level, 0, 0, 0, &fmt, SrcData,
                                                nullptr, nullptr) == 0
             ? GR_OK
             : GR_GENERIC;
  (void)SrcSize;
}

GENRESULT TextureAdapter::GetImageInfo(_D3DXIMAGE_INFO* Info, unsigned int SrcSize,
                                      const void* SrcData) {
  (void)Info; (void)SrcSize; (void)SrcData;
  return GR_GENERIC;
}

GENRESULT TextureAdapter::LoadCubemap(IFileSystem* /*FileSystem*/, const char* FileName,
                                      void** OutHandle) {
  if (!rp_ || !FileName || !OutHandle) return GR_INVALID_PARMS;
  return rp_->textures().load_texture(FileName, OutHandle) == 0 ? GR_OK : GR_GENERIC;
}

}  // namespace fl_port

#endif
