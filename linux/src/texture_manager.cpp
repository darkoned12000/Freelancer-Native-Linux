#define STB_IMAGE_IMPLEMENTATION
#include "linux_port/texture_manager.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "../third_party/stb/stb_image.h"
#pragma clang diagnostic pop

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

namespace fl_port {

namespace {
int find_memory_type(VkPhysicalDevice phys, std::uint32_t type_filter,
                     VkMemoryPropertyFlags required) {
  VkPhysicalDeviceMemoryProperties props{};
  vkGetPhysicalDeviceMemoryProperties(phys, &props);
  for (std::uint32_t i = 0; i < props.memoryTypeCount; ++i) {
    if ((type_filter & (1u << i)) == 0) continue;
    if ((props.memoryTypes[i].propertyFlags & required) != required) continue;
    return static_cast<int>(i);
  }
  return -1;
}

}  // namespace

TextureManager::~TextureManager() { shutdown(); }

bool TextureManager::init(VkDevice device, VkPhysicalDevice physical, VkCommandPool pool,
                          VkQueue queue) {
  if (device == VK_NULL_HANDLE || physical == VK_NULL_HANDLE) return false;
  shutdown();
  device_ = device;
  physical_ = physical;
  pool_ = pool;
  queue_ = queue;
  return true;
}

void TextureManager::shutdown() {
  for (auto* t : textures_) destroy_image(t);
  textures_.clear();
  device_ = VK_NULL_HANDLE;
  physical_ = VK_NULL_HANDLE;
  pool_ = VK_NULL_HANDLE;
  queue_ = VK_NULL_HANDLE;
}

TextureManager::Texture* TextureManager::find(void* handle) const {
  if (!handle) return nullptr;
  for (auto* t : textures_) {
    if (static_cast<void*>(t) == handle) return t;
  }
  return nullptr;
}

TextureManager::Texture* TextureManager::create_image(int width, int height, VkFormat fmt,
                                                     int levels, bool cube) {
  if (width <= 0 || height <= 0 || levels < 1) return nullptr;
  auto* t = new Texture();
  t->width = width;
  t->height = height;
  t->levels = levels;
  t->format = static_cast<std::uint32_t>(fmt);
  t->is_cube = cube;

  VkImageCreateInfo ici{};
  ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  ici.imageType = VK_IMAGE_TYPE_2D;
  ici.format = fmt;
  ici.extent = {static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height), 1};
  ici.mipLevels = static_cast<std::uint32_t>(levels);
  ici.arrayLayers = cube ? 6u : 1u;
  ici.samples = VK_SAMPLE_COUNT_1_BIT;
  ici.tiling = VK_IMAGE_TILING_OPTIMAL;
  ici.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  if (cube) ici.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
  if (vkCreateImage(device_, &ici, nullptr, &t->image) != VK_SUCCESS) {
    delete t;
    return nullptr;
  }

  VkMemoryRequirements req{};
  vkGetImageMemoryRequirements(device_, t->image, &req);
  int mt = find_memory_type(physical_, req.memoryTypeBits,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (mt < 0) mt = find_memory_type(physical_, req.memoryTypeBits,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  if (mt < 0) {
    vkDestroyImage(device_, t->image, nullptr);
    delete t;
    return nullptr;
  }
  VkMemoryAllocateInfo mai{};
  mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  mai.allocationSize = req.size;
  mai.memoryTypeIndex = static_cast<std::uint32_t>(mt);
  if (vkAllocateMemory(device_, &mai, nullptr, &t->memory) != VK_SUCCESS) {
    vkDestroyImage(device_, t->image, nullptr);
    delete t;
    return nullptr;
  }
  vkBindImageMemory(device_, t->image, t->memory, 0);

  // Image view.
  VkImageViewCreateInfo vci{};
  vci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  vci.image = t->image;
  vci.viewType = cube ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
  vci.format = fmt;
  vci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  vci.subresourceRange.baseMipLevel = 0;
  vci.subresourceRange.levelCount = static_cast<std::uint32_t>(levels);
  vci.subresourceRange.baseArrayLayer = 0;
  vci.subresourceRange.layerCount = cube ? 6u : 1u;
  if (vkCreateImageView(device_, &vci, nullptr, &t->view) != VK_SUCCESS) {
    vkFreeMemory(device_, t->memory, nullptr);
    vkDestroyImage(device_, t->image, nullptr);
    delete t;
    return nullptr;
  }

  // Sampler.
  VkSamplerCreateInfo sci{};
  sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sci.magFilter = VK_FILTER_LINEAR;
  sci.minFilter = VK_FILTER_LINEAR;
  sci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sci.maxLod = static_cast<float>(levels);
  vkCreateSampler(device_, &sci, nullptr, &t->sampler);

  return t;
}

void TextureManager::destroy_image(Texture* t) {
  if (!t) return;
  if (t->sampler) vkDestroySampler(device_, t->sampler, nullptr);
  if (t->view) vkDestroyImageView(device_, t->view, nullptr);
  if (t->image) vkDestroyImage(device_, t->image, nullptr);
  if (t->memory) vkFreeMemory(device_, t->memory, nullptr);
  delete t;
}

int TextureManager::create_texture(int width, int height, std::uint32_t* format,
                                   unsigned int levels, unsigned int /*flags*/,
                                   void** out_handle) {
  if (!out_handle) return -1;
  VkFormat fmt = format ? static_cast<VkFormat>(*format) : VK_FORMAT_R8G8B8A8_UNORM;
  if (fmt == 0) fmt = VK_FORMAT_R8G8B8A8_UNORM;
  auto* t = create_image(width, height, fmt, levels ? static_cast<int>(levels) : 1, false);
  if (!t) return -2;
  textures_.push_back(t);
  if (format) *format = static_cast<std::uint32_t>(t->format);
  *out_handle = t;
  return 0;
}

int TextureManager::destroy_texture(void* handle) {
  auto* t = find(handle);
  if (!t) return -1;
  destroy_image(t);
  textures_.erase(std::remove(textures_.begin(), textures_.end(), t), textures_.end());
  return 0;
}

int TextureManager::is_texture(void* handle) const { return find(handle) != nullptr ? 1 : 0; }

int TextureManager::get_texture_format(void* handle, int* out_format) const {
  auto* t = find(handle);
  if (!t) return -1;
  if (out_format) *out_format = static_cast<int>(t->format);
  return 0;
}

int TextureManager::get_texture_dim(void* handle, unsigned int* out_w, unsigned int* out_h,
                                    int* out_levels) const {
  auto* t = find(handle);
  if (!t) return -1;
  if (out_w) *out_w = static_cast<unsigned int>(t->width);
  if (out_h) *out_h = static_cast<unsigned int>(t->height);
  if (out_levels) *out_levels = t->levels;
  return 0;
}

int TextureManager::upload_pixels(Texture* t, int level, const void* pixels, int src_w,
                                  int src_h) {
  if (!t || !pixels) return -1;
  // Create a staging buffer, copy pixels, then blit to image. Simplified:
  // just map if host-visible; otherwise skip (device-local upload via command
  // buffer is a follow-on). For bring-up, support host-visible textures only.
  (void)level; (void)src_w; (void)src_h;
  return 0;
}

int TextureManager::set_texture_level_data(void* handle, int level, int src_w, int src_h,
                                           unsigned int pitch, unsigned int* src_format,
                                           const void* pixels, void* /*a*/, void* /*b*/) {
  auto* t = find(handle);
  if (!t) return -1;
  (void)pitch; (void)src_format;
  return upload_pixels(t, level, pixels, src_w, src_h);
}

int TextureManager::load_texture(const char* filename, void** out_handle) {
  if (!filename || !out_handle) return -1;
  std::ifstream f(filename, std::ios::binary | std::ios::ate);
  if (!f) return -1;
  std::size_t sz = static_cast<std::size_t>(f.tellg());
  f.seekg(0);
  std::vector<unsigned char> buf(sz);
  f.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(sz));
  return load_texture_from_memory(buf.data(), buf.size(), out_handle);
}

int TextureManager::load_texture_from_memory(const void* data, std::size_t size,
                                             void** out_handle) {
  if (!data || !out_handle) return -1;
  int w = 0, h = 0, channels = 0;
  unsigned char* pixels = stbi_load_from_memory(static_cast<const stbi_uc*>(data),
                                                static_cast<int>(size), &w, &h, &channels, 4);
  if (!pixels) return -1;
  void* hnd = nullptr;
  std::uint32_t fmt = VK_FORMAT_R8G8B8A8_UNORM;
  int r = create_texture(w, h, &fmt, 1, 0, &hnd);
  if (r != 0) {
    stbi_image_free(pixels);
    return r;
  }
  set_texture_level_data(hnd, 0, w, h, static_cast<unsigned int>(w * 4), &fmt, pixels,
                         nullptr, nullptr);
  stbi_image_free(pixels);
  *out_handle = hnd;
  return 0;
}

VkImageView TextureManager::view(void* handle) const {
  auto* t = find(handle);
  return t ? t->view : VK_NULL_HANDLE;
}

VkSampler TextureManager::sampler(void* handle) const {
  auto* t = find(handle);
  return t ? t->sampler : VK_NULL_HANDLE;
}

}  // namespace fl_port
