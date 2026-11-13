#if defined(FL_LINUX_32BIT)

#include "linux_port/vb_adapter.h"

namespace fl_port {

GENRESULT VbAdapter::LockVertexMemory(unsigned int Fvf, unsigned int Count,
                                     FVertexMemoryLock* Out) {
  if (!rp_ || !Out) return GR_INVALID_PARMS;
  void* h = nullptr;
  int r = rp_->vertex_buffers().create_vb(Fvf, Count, 0, &h);
  if (r != 0 || !h) return GR_GENERIC;
  void* ptr = nullptr;
  std::uint32_t off = 0;
  rp_->vertex_buffers().lock_vb(h, &off, &ptr, Count);
  Out->Listing.VertexBuffer = h;
  Out->Listing.Fvf = Fvf;
  Out->Listing.BaseVertex = 0;
  Out->Vertices = ptr;
  return GR_OK;
}

GENRESULT VbAdapter::UnlockVertexMemory(FVertexMemoryLock* Out) {
  if (!rp_ || !Out) return GR_INVALID_PARMS;
  void* h = Out->Listing.VertexBuffer;
  return rp_->vertex_buffers().unlock_vb(h) == 0 ? GR_OK : GR_GENERIC;
}

GENRESULT VbAdapter::CopyVertexData(void* Dest, unsigned int Fvf,
                                    const FRpVertexSrcDesc* Source,
                                    unsigned int StartVertex, unsigned int Count) {
  if (!rp_) return GR_GENERIC;
  return rp_->vertex_buffers().copy_vertices(Dest, nullptr, Source, StartVertex, Count) == 0
             ? GR_OK
             : GR_GENERIC;
}

GENRESULT VbAdapter::CreateIb(unsigned int Count, unsigned int** Out, unsigned int Flags) {
  if (!rp_ || !Out) return GR_INVALID_PARMS;
  return rp_->vertex_buffers().create_ib(Count, Out, Flags) == 0 ? GR_OK : GR_GENERIC;
}

GENRESULT VbAdapter::DestroyIb(void** Handle) {
  if (!rp_) return GR_GENERIC;
  return rp_->vertex_buffers().destroy_ib(Handle) == 0 ? GR_OK : GR_GENERIC;
}

GENRESULT VbAdapter::ResizeIb(void* Handle, unsigned int Count) {
  if (!rp_) return GR_GENERIC;
  return rp_->vertex_buffers().resize_ib(Handle, Count) == 0 ? GR_OK : GR_GENERIC;
}

GENRESULT VbAdapter::CopyIndices(void* Handle, unsigned int* OutOffset,
                                 const unsigned short* SrcData, unsigned int Count) {
  if (!rp_) return GR_GENERIC;
  return rp_->vertex_buffers().copy_indices(Handle, OutOffset, SrcData, Count) == 0 ? GR_OK
                                                                                    : GR_GENERIC;
}

GENRESULT VbAdapter::LockIb(void* Handle, unsigned int* Offset, void** Out,
                            unsigned int Count) {
  if (!rp_) return GR_GENERIC;
  return rp_->vertex_buffers().lock_ib(Handle, Offset, Out, Count) == 0 ? GR_OK : GR_GENERIC;
}

GENRESULT VbAdapter::UnlockIb(void* Handle) {
  if (!rp_) return GR_GENERIC;
  return rp_->vertex_buffers().unlock_ib(Handle) == 0 ? GR_OK : GR_GENERIC;
}

GENRESULT VbAdapter::SelectIb(void* Handle, unsigned int BaseVertexIndex,
                              unsigned int /*Unused3*/, unsigned int /*Unused4*/) {
  if (!rp_) return GR_GENERIC;
  return rp_->vertex_buffers().select_ib(Handle, BaseVertexIndex) == 0 ? GR_OK : GR_GENERIC;
}

GENRESULT VbAdapter::GetIbCapacity(void* Handle, unsigned int* OutCapacity) {
  if (!rp_) return GR_GENERIC;
  return rp_->vertex_buffers().get_ib_capacity(Handle, OutCapacity) == 0 ? GR_OK : GR_GENERIC;
}

GENRESULT VbAdapter::IsIbValid(void* Handle) {
  if (!rp_) return GR_GENERIC;
  return rp_->vertex_buffers().is_ib_valid(Handle) ? GR_OK : GR_GENERIC;
}

GENRESULT VbAdapter::CreateVb(unsigned int Fvf, unsigned int Count, unsigned int** Out,
                               unsigned int Flags) {
  if (!rp_ || !Out) return GR_INVALID_PARMS;
  void* h = nullptr;
  int r = rp_->vertex_buffers().create_vb(Fvf, Count, Flags, &h);
  *Out = reinterpret_cast<unsigned int*>(h);
  return r == 0 ? GR_OK : GR_GENERIC;
}

GENRESULT VbAdapter::DestroyVb(void** Handle) {
  if (!rp_) return GR_GENERIC;
  return rp_->vertex_buffers().destroy_vb(Handle) == 0 ? GR_OK : GR_GENERIC;
}

GENRESULT VbAdapter::ResizeVb(void* Handle, unsigned int Fvf, unsigned int Count) {
  if (!rp_) return GR_GENERIC;
  return rp_->vertex_buffers().resize_vb(Handle, Fvf, Count) == 0 ? GR_OK : GR_GENERIC;
}

GENRESULT VbAdapter::CopyVertices(void* Handle, unsigned int* OutOffset,
                                  const FRpVertexSrcDesc* SrcData,
                                  unsigned int StartVertex, unsigned int Count) {
  if (!rp_) return GR_GENERIC;
  return rp_->vertex_buffers().copy_vertices(Handle, OutOffset, SrcData, StartVertex, Count) == 0
             ? GR_OK
             : GR_GENERIC;
}

GENRESULT VbAdapter::LockVb(void* Handle, unsigned int* Offset, void** Out,
                            unsigned int Count) {
  if (!rp_) return GR_GENERIC;
  return rp_->vertex_buffers().lock_vb(Handle, Offset, Out, Count) == 0 ? GR_OK : GR_GENERIC;
}

GENRESULT VbAdapter::UnlockVb(void* Handle) {
  if (!rp_) return GR_GENERIC;
  return rp_->vertex_buffers().unlock_vb(Handle) == 0 ? GR_OK : GR_GENERIC;
}

GENRESULT VbAdapter::SelectVb(void* Handle) {
  if (!rp_) return GR_GENERIC;
  return rp_->vertex_buffers().select_vb(Handle) == 0 ? GR_OK : GR_GENERIC;
}

GENRESULT VbAdapter::GetVbInfo(void* Handle, unsigned int* OutFvf, unsigned int* OutCount) {
  if (!rp_) return GR_GENERIC;
  return rp_->vertex_buffers().get_vb_info(Handle, OutFvf, OutCount) == 0 ? GR_OK : GR_GENERIC;
}

GENRESULT VbAdapter::IsVbValid(void* Handle) {
  if (!rp_) return GR_GENERIC;
  return rp_->vertex_buffers().is_vb_valid(Handle) ? GR_OK : GR_GENERIC;
}

}  // namespace fl_port

#endif  // FL_LINUX_32BIT
