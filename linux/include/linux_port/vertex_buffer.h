#pragma once

// M9.1: native Vulkan vertex/index buffer management for the Linux port.
// Provides create/lock/unlock/select/copy for vertex and index buffers backed
// by host-visible, host-coherent VkBuffers. This is the foundation that the
// upstream IRenderPipeline vertex-buffer interfaces (IRPVertexBuffer,
// IRPIndexBuffer, IVertexBufferManager) will be adapted onto.

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace fl_port {

class VertexBufferManager {
 public:
  VertexBufferManager() = default;
  ~VertexBufferManager();

  VertexBufferManager(const VertexBufferManager&) = delete;
  VertexBufferManager& operator=(const VertexBufferManager&) = delete;

  // Initialize with a Vulkan device + physical device. Returns true on success.
  bool init(VkDevice device, VkPhysicalDevice physical);
  void shutdown();

  bool is_initialized() const { return device_ != VK_NULL_HANDLE; }

  // ---- Vertex buffers ----
  // CreateVb-equivalent: allocate a host-visible buffer for `count` vertices of
  // the given FVF. Handle is an opaque pointer (VkBuffer).
  int create_vb(std::uint32_t fvf, std::uint32_t count, std::uint32_t flags,
                void** out_handle);
  int destroy_vb(void** handle);
  int resize_vb(void* handle, std::uint32_t fvf, std::uint32_t count);
  int lock_vb(void* handle, std::uint32_t* out_offset, void** out_ptr,
              std::uint32_t count);
  int unlock_vb(void* handle);
  int select_vb(void* handle);
  int get_vb_info(void* handle, std::uint32_t* out_fvf, std::uint32_t* out_count) const;
  int is_vb_valid(void* handle) const;

  // CopyVertices-equivalent: copy from an FRpVertexSrcDesc-style source into the
  // destination buffer at StartVertex. Returns GR_OK on success.
  int copy_vertices(void* handle, std::uint32_t* out_offset,
                    const void* src_desc, std::uint32_t start_vertex,
                    std::uint32_t count);

  // ---- Index buffers ----
  int create_ib(std::uint32_t count, std::uint32_t** out_handle, std::uint32_t flags);
  int destroy_ib(void** handle);
  int resize_ib(void* handle, std::uint32_t count);
  int lock_ib(void* handle, std::uint32_t* out_offset, void** out_ptr,
              std::uint32_t count);
  int unlock_ib(void* handle);
  int select_ib(void* handle, std::uint32_t base_vertex_index);
  int get_ib_capacity(void* handle, std::uint32_t* out_capacity) const;
  int is_ib_valid(void* handle) const;
  int copy_indices(void* handle, std::uint32_t* out_offset,
                   const std::uint16_t* src, std::uint32_t count);

  // Currently selected handles (for diagnostics).
  void* selected_vb() const { return selected_vb_; }
  void* selected_ib() const { return selected_ib_; }

 private:
  struct Buffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mapped = nullptr;
    std::uint32_t fvf = 0;
    std::uint32_t count = 0;
    std::uint32_t stride = 0;
    bool is_index = false;
    std::uint32_t capacity_bytes = 0;
  };

  Buffer* find(void* handle) const;
  Buffer* create_buffer(std::uint32_t size_bytes, bool index);
  void destroy_buffer(Buffer* b);

  VkDevice device_ = VK_NULL_HANDLE;
  VkPhysicalDevice physical_ = VK_NULL_HANDLE;
  std::vector<Buffer*> buffers_;
  void* selected_vb_ = nullptr;
  void* selected_ib_ = nullptr;
  std::uint32_t selected_ib_base_ = 0;
};

}  // namespace fl_port
