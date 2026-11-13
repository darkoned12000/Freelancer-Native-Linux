#include "linux_port/vertex_buffer.h"
#if defined(FL_LINUX_32BIT)
#include "render/IRenderPipeline.h"
#endif

#include <cstring>
#include <algorithm>
#include <iostream>

namespace fl_port {

namespace {
// Find a memory type index satisfying required + preferred flags.
int find_memory_type(VkPhysicalDevice phys, std::uint32_t type_filter,
                     VkMemoryPropertyFlags required,
                     VkMemoryPropertyFlags preferred) {
  VkPhysicalDeviceMemoryProperties props{};
  vkGetPhysicalDeviceMemoryProperties(phys, &props);
  // First try required+preferred.
  for (std::uint32_t i = 0; i < props.memoryTypeCount; ++i) {
    if ((type_filter & (1u << i)) == 0) continue;
    if ((props.memoryTypes[i].propertyFlags & required) != required) continue;
    if ((props.memoryTypes[i].propertyFlags & preferred) != preferred) continue;
    return static_cast<int>(i);
  }
  // Fall back to required only.
  for (std::uint32_t i = 0; i < props.memoryTypeCount; ++i) {
    if ((type_filter & (1u << i)) == 0) continue;
    if ((props.memoryTypes[i].propertyFlags & required) != required) continue;
    return static_cast<int>(i);
  }
  return -1;
}

// D3D8 FVF masks (from d3d8types.h).
static constexpr std::uint32_t D3DFVF_XYZ = 0x0002;
static constexpr std::uint32_t D3DFVF_XYZRHW = 0x0004;
static constexpr std::uint32_t D3DFVF_NORMAL = 0x0010;
static constexpr std::uint32_t D3DFVF_PSIZE = 0x0020;
static constexpr std::uint32_t D3DFVF_DIFFUSE = 0x0040;
static constexpr std::uint32_t D3DFVF_SPECULAR = 0x0080;
static constexpr std::uint32_t D3DFVF_TEXCOUNT_MASK = 0x0f00;
static constexpr std::uint32_t D3DFVF_TEXCOUNT_SHIFT = 8;

std::uint32_t fvf_stride(std::uint32_t fvf) {
  std::uint32_t stride = 0;
  // Position.
  std::uint32_t pos = fvf & 0x000E;
  if (pos == D3DFVF_XYZ) stride += 3 * sizeof(float);
  else if (pos == D3DFVF_XYZRHW) stride += 4 * sizeof(float);
  else if (pos == 0x0006) stride += 3 * sizeof(float) + 1 * sizeof(float); // XYZB1
  else if (pos == 0x0008) stride += 3 * sizeof(float) + 2 * sizeof(float); // XYZB2
  else if (pos == 0x000a) stride += 3 * sizeof(float) + 3 * sizeof(float); // XYZB3
  else if (pos == 0x000c) stride += 3 * sizeof(float) + 4 * sizeof(float); // XYZB4
  else if (pos == 0x000e) stride += 3 * sizeof(float) + 5 * sizeof(float); // XYZB5
  else stride += 3 * sizeof(float); // default XYZ
  if (fvf & D3DFVF_NORMAL) stride += 3 * sizeof(float);
  if (fvf & D3DFVF_PSIZE) stride += sizeof(float);
  if (fvf & D3DFVF_DIFFUSE) stride += sizeof(std::uint32_t);
  if (fvf & D3DFVF_SPECULAR) stride += sizeof(std::uint32_t);
  std::uint32_t tex_count = (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
  // Default 2 floats (8 bytes) per texcoord set.
  stride += tex_count * 2 * sizeof(float);
  if (stride == 0) stride = 32;
  return stride;
}
}  // namespace

VertexBufferManager::~VertexBufferManager() { shutdown(); }

bool VertexBufferManager::init(VkDevice device, VkPhysicalDevice physical) {
  if (device == VK_NULL_HANDLE || physical == VK_NULL_HANDLE) return false;
  shutdown();
  device_ = device;
  physical_ = physical;
  return true;
}

void VertexBufferManager::shutdown() {
  for (auto* b : buffers_) destroy_buffer(b);
  buffers_.clear();
  selected_vb_ = nullptr;
  selected_ib_ = nullptr;
  device_ = VK_NULL_HANDLE;
  physical_ = VK_NULL_HANDLE;
}

VertexBufferManager::Buffer* VertexBufferManager::find(void* handle) const {
  if (!handle) return nullptr;
  for (auto* b : buffers_) {
    if (static_cast<void*>(b) == handle) return b;
  }
  return nullptr;
}

VertexBufferManager::Buffer* VertexBufferManager::create_buffer(std::uint32_t size_bytes,
                                                              bool index) {
  if (device_ == VK_NULL_HANDLE || size_bytes == 0) return nullptr;
  auto* b = new Buffer();
  b->is_index = index;
  b->capacity_bytes = size_bytes;

  VkBufferCreateInfo bci{};
  bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bci.size = size_bytes;
  bci.usage = index ? VK_BUFFER_USAGE_INDEX_BUFFER_BIT : VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if (vkCreateBuffer(device_, &bci, nullptr, &b->buffer) != VK_SUCCESS) {
    delete b;
    return nullptr;
  }

  VkMemoryRequirements req{};
  vkGetBufferMemoryRequirements(device_, b->buffer, &req);
  int mt = find_memory_type(physical_, req.memoryTypeBits,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            0);
  if (mt < 0) {
    vkDestroyBuffer(device_, b->buffer, nullptr);
    delete b;
    return nullptr;
  }
  VkMemoryAllocateInfo mai{};
  mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  mai.allocationSize = req.size;
  mai.memoryTypeIndex = static_cast<std::uint32_t>(mt);
  if (vkAllocateMemory(device_, &mai, nullptr, &b->memory) != VK_SUCCESS) {
    vkDestroyBuffer(device_, b->buffer, nullptr);
    delete b;
    return nullptr;
  }
  if (vkBindBufferMemory(device_, b->buffer, b->memory, 0) != VK_SUCCESS) {
    vkFreeMemory(device_, b->memory, nullptr);
    vkDestroyBuffer(device_, b->buffer, nullptr);
    delete b;
    return nullptr;
  }
  // Persistently map (host-coherent).
  if (vkMapMemory(device_, b->memory, 0, VK_WHOLE_SIZE, 0, &b->mapped) != VK_SUCCESS) {
    b->mapped = nullptr;
  }
  return b;
}

void VertexBufferManager::destroy_buffer(Buffer* b) {
  if (!b) return;
  if (b->mapped) vkUnmapMemory(device_, b->memory);
  if (b->buffer) vkDestroyBuffer(device_, b->buffer, nullptr);
  if (b->memory) vkFreeMemory(device_, b->memory, nullptr);
  delete b;
}

int VertexBufferManager::create_vb(std::uint32_t fvf, std::uint32_t count,
                                   std::uint32_t /*flags*/, void** out_handle) {
  if (!out_handle) return -1;  // GR_INVALID_PARMS-ish
  std::uint32_t stride = fvf_stride(fvf);
  std::uint32_t bytes = stride * (count ? count : 1);
  auto* b = create_buffer(bytes, false);
  if (!b) return -2;
  b->fvf = fvf;
  b->count = count;
  b->stride = stride;
  buffers_.push_back(b);
  *out_handle = b;
  return 0;  // GR_OK
}

int VertexBufferManager::destroy_vb(void** handle) {
  if (!handle) return -1;
  auto* b = find(*handle);
  if (!b) return -1;
  destroy_buffer(b);
  buffers_.erase(std::remove(buffers_.begin(), buffers_.end(), b), buffers_.end());
  if (selected_vb_ == *handle) selected_vb_ = nullptr;
  *handle = nullptr;
  return 0;
}

int VertexBufferManager::resize_vb(void* handle, std::uint32_t fvf, std::uint32_t count) {
  auto* b = find(handle);
  if (!b) return -1;
  std::uint32_t stride = fvf_stride(fvf);
  std::uint32_t bytes = stride * (count ? count : 1);
  // Recreate.
  VkBuffer old_buf = b->buffer;
  VkDeviceMemory old_mem = b->memory;
  void* old_mapped = b->mapped;
  auto* nb = create_buffer(bytes, false);
  if (!nb) return -2;
  // copy old data up to min size
  std::uint32_t old_bytes = b->capacity_bytes;
  std::uint32_t copy = old_bytes < bytes ? old_bytes : bytes;
  if (old_mapped && nb->mapped && copy) std::memcpy(nb->mapped, old_mapped, copy);
  // destroy old
  if (old_mapped) vkUnmapMemory(device_, old_mem);
  if (old_buf) vkDestroyBuffer(device_, old_buf, nullptr);
  if (old_mem) vkFreeMemory(device_, old_mem, nullptr);
  b->buffer = nb->buffer;
  b->memory = nb->memory;
  b->mapped = nb->mapped;
  b->capacity_bytes = nb->capacity_bytes;
  nb->buffer = VK_NULL_HANDLE;
  nb->memory = VK_NULL_HANDLE;
  nb->mapped = nullptr;
  delete nb;
  b->fvf = fvf;
  b->count = count;
  b->stride = stride;
  return 0;
}

int VertexBufferManager::lock_vb(void* handle, std::uint32_t* out_offset, void** out_ptr,
                                std::uint32_t /*count*/) {
  auto* b = find(handle);
  if (!b || !b->mapped) return -1;
  if (out_offset) *out_offset = 0;
  if (out_ptr) *out_ptr = b->mapped;
  return 0;
}

int VertexBufferManager::unlock_vb(void* handle) {
  auto* b = find(handle);
  if (!b) return -1;
  // Host-coherent: no flush needed, but flush for safety.
  if (b->mapped) {
    VkMappedMemoryRange r{};
    r.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    r.memory = b->memory;
    r.offset = 0;
    r.size = VK_WHOLE_SIZE;
    vkFlushMappedMemoryRanges(device_, 1, &r);
  }
  return 0;
}

int VertexBufferManager::select_vb(void* handle) {
  if (handle && !find(handle)) return -1;
  selected_vb_ = handle;
  return 0;
}

int VertexBufferManager::get_vb_info(void* handle, std::uint32_t* out_fvf,
                                     std::uint32_t* out_count) const {
  auto* b = find(handle);
  if (!b) return -1;
  if (out_fvf) *out_fvf = b->fvf;
  if (out_count) *out_count = b->count;
  return 0;
}

int VertexBufferManager::is_vb_valid(void* handle) const {
  return find(handle) != nullptr ? 1 : 0;
}

int VertexBufferManager::copy_vertices(void* handle, std::uint32_t* out_offset,
                                      const void* src_desc, std::uint32_t start_vertex,
                                      std::uint32_t count) {
  auto* b = find(handle);
  if (!b || !b->mapped || !src_desc) return -1;
  std::uint32_t off = start_vertex * b->stride;
#if defined(FL_LINUX_32BIT)
  const auto* desc = static_cast<const FRpVertexSrcDesc*>(src_desc);
  if (desc) {
    // Prefer the position stream as the source of vertex data. If the source
    // is interleaved (flags & 2), the position stream's Base points at the
    // first attribute of each vertex with Stride bytes between vertices.
    const FRpVertexStream& pos = desc->Position;
    std::uint32_t src_stride = pos.Stride ? pos.Stride : b->stride;
    if (pos.Base && count > 0) {
      if (off + count * b->stride > b->capacity_bytes) return -1;
      if (src_stride == b->stride) {
        std::memcpy(static_cast<std::uint8_t*>(b->mapped) + off, pos.Base,
                    count * b->stride);
      } else {
        // Stride mismatch: copy element-wise (position only, 12 bytes) then
        // zero-fill the rest of each vertex. Best-effort bring-up path.
        for (std::uint32_t i = 0; i < count; ++i) {
          std::uint8_t* dst = static_cast<std::uint8_t*>(b->mapped) + off + i * b->stride;
          std::memset(dst, 0, b->stride);
          std::memcpy(dst, pos.Base + i * src_stride, 12); // xyz
        }
      }
      if (out_offset) *out_offset = off;
      return 0;
    }
  }
#endif
  // Fallback: treat src_desc as a flat vertex array.
  const std::uint8_t* src = static_cast<const std::uint8_t*>(src_desc);
  if (off + count * b->stride > b->capacity_bytes) return -1;
  std::memcpy(static_cast<std::uint8_t*>(b->mapped) + off, src, count * b->stride);
  if (out_offset) *out_offset = off;
  return 0;
}

// ---- Index buffers ----

int VertexBufferManager::create_ib(std::uint32_t count, std::uint32_t** out_handle,
                                   std::uint32_t /*flags*/) {
  if (!out_handle) return -1;
  std::uint32_t bytes = sizeof(std::uint16_t) * (count ? count : 1);
  auto* b = create_buffer(bytes, true);
  if (!b) return -2;
  b->count = count;
  b->stride = sizeof(std::uint16_t);
  buffers_.push_back(b);
  *out_handle = reinterpret_cast<std::uint32_t*>(b);
  return 0;
}

int VertexBufferManager::destroy_ib(void** handle) {
  if (!handle) return -1;
  auto* b = find(*handle);
  if (!b) return -1;
  destroy_buffer(b);
  buffers_.erase(std::remove(buffers_.begin(), buffers_.end(), b), buffers_.end());
  if (selected_ib_ == *handle) selected_ib_ = nullptr;
  *handle = nullptr;
  return 0;
}

int VertexBufferManager::resize_ib(void* handle, std::uint32_t count) {
  auto* b = find(handle);
  if (!b) return -1;
  std::uint32_t bytes = sizeof(std::uint16_t) * (count ? count : 1);
  // Recreate.
  VkBuffer old_buf = b->buffer;
  VkDeviceMemory old_mem = b->memory;
  void* old_mapped = b->mapped;
  auto* nb = create_buffer(bytes, true);
  if (!nb) return -2;
  std::uint32_t old_bytes = b->capacity_bytes;
  std::uint32_t copy = old_bytes < bytes ? old_bytes : bytes;
  if (old_mapped && nb->mapped && copy) std::memcpy(nb->mapped, old_mapped, copy);
  if (old_mapped) vkUnmapMemory(device_, old_mem);
  if (old_buf) vkDestroyBuffer(device_, old_buf, nullptr);
  if (old_mem) vkFreeMemory(device_, old_mem, nullptr);
  b->buffer = nb->buffer;
  b->memory = nb->memory;
  b->mapped = nb->mapped;
  b->capacity_bytes = nb->capacity_bytes;
  nb->buffer = VK_NULL_HANDLE;
  nb->memory = VK_NULL_HANDLE;
  nb->mapped = nullptr;
  delete nb;
  b->count = count;
  return 0;
}

int VertexBufferManager::lock_ib(void* handle, std::uint32_t* out_offset, void** out_ptr,
                                std::uint32_t /*count*/) {
  auto* b = find(handle);
  if (!b || !b->mapped) return -1;
  if (out_offset) *out_offset = 0;
  if (out_ptr) *out_ptr = b->mapped;
  return 0;
}

int VertexBufferManager::unlock_ib(void* handle) {
  auto* b = find(handle);
  if (!b) return -1;
  if (b->mapped) {
    VkMappedMemoryRange r{};
    r.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    r.memory = b->memory;
    r.offset = 0;
    r.size = VK_WHOLE_SIZE;
    vkFlushMappedMemoryRanges(device_, 1, &r);
  }
  return 0;
}

int VertexBufferManager::select_ib(void* handle, std::uint32_t base_vertex_index) {
  if (handle && !find(handle)) return -1;
  selected_ib_ = handle;
  selected_ib_base_ = base_vertex_index;
  return 0;
}

int VertexBufferManager::get_ib_capacity(void* handle, std::uint32_t* out_capacity) const {
  auto* b = find(handle);
  if (!b) return -1;
  if (out_capacity) *out_capacity = b->count;
  return 0;
}

int VertexBufferManager::is_ib_valid(void* handle) const {
  return find(handle) != nullptr ? 1 : 0;
}

int VertexBufferManager::copy_indices(void* handle, std::uint32_t* /*out_offset*/,
                                      const std::uint16_t* src, std::uint32_t count) {
  auto* b = find(handle);
  if (!b || !b->mapped || !src) return -1;
  std::uint32_t bytes = count * sizeof(std::uint16_t);
  if (bytes > b->capacity_bytes) return -1;
  std::memcpy(b->mapped, src, bytes);
  return 0;
}

}  // namespace fl_port
