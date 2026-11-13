#pragma once

// M9.2: native Vulkan texture loading/binding for the Linux port.

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace fl_port {

class TextureManager {
 public:
  TextureManager() = default;
  ~TextureManager();

  TextureManager(const TextureManager&) = delete;
  TextureManager& operator=(const TextureManager&) = delete;

  bool init(VkDevice device, VkPhysicalDevice physical, VkCommandPool pool, VkQueue queue);
  void shutdown();
  bool is_initialized() const { return device_ != VK_NULL_HANDLE; }

  struct Texture {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    int width = 0;
    int height = 0;
    int levels = 1;
    std::uint32_t format = 0; // VkFormat
    bool is_cube = false;
  };

  int create_texture(int width, int height, std::uint32_t* format, unsigned int levels,
                     unsigned int flags, void** out_handle);
  int destroy_texture(void* handle);
  int is_texture(void* handle) const;
  int get_texture_format(void* handle, int* out_format) const;
  int get_texture_dim(void* handle, unsigned int* out_w, unsigned int* out_h, int* out_levels) const;

  // Load from a file path on disk (PNG/JPG/DDS via stb for PNG/JPG).
  int load_texture(const char* filename, void** out_handle);
  // Load from memory buffer.
  int load_texture_from_memory(const void* data, std::size_t size, void** out_handle);

  // Upload level data.
  int set_texture_level_data(void* handle, int level, int src_w, int src_h,
                             unsigned int pitch, unsigned int* src_format,
                             const void* pixels, void* a, void* b);

  VkImageView view(void* handle) const;
  VkSampler sampler(void* handle) const;

 private:
  Texture* find(void* handle) const;
  Texture* create_image(int width, int height, VkFormat fmt, int levels, bool cube);
  void destroy_image(Texture* t);
  int upload_pixels(Texture* t, int level, const void* pixels, int src_w, int src_h);

  VkDevice device_ = VK_NULL_HANDLE;
  VkPhysicalDevice physical_ = VK_NULL_HANDLE;
  VkCommandPool pool_ = VK_NULL_HANDLE;
  VkQueue queue_ = VK_NULL_HANDLE;
  std::vector<Texture*> textures_;
};

}  // namespace fl_port
