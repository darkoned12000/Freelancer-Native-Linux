#include "linux_port/font_manager.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
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

FontManager::~FontManager() { shutdown(); }

bool FontManager::init(VkDevice device, VkPhysicalDevice physical, VkCommandPool pool,
                       VkQueue queue) {
  if (device == VK_NULL_HANDLE || physical == VK_NULL_HANDLE) return false;
  shutdown();
  device_ = device;
  physical_ = physical;
  pool_ = pool;
  queue_ = queue;
  if (FT_Init_FreeType(&ft_lib_) != 0) {
    ft_lib_ = nullptr;
    return false;
  }
  return true;
}

void FontManager::shutdown() {
  for (auto& f : fonts_) {
    if (f.face) FT_Done_Face(f.face);
  }
  fonts_.clear();
  if (atlas_view_) vkDestroyImageView(device_, atlas_view_, nullptr);
  atlas_view_ = VK_NULL_HANDLE;
  if (atlas_sampler_) vkDestroySampler(device_, atlas_sampler_, nullptr);
  atlas_sampler_ = VK_NULL_HANDLE;
  if (atlas_image_) vkDestroyImage(device_, atlas_image_, nullptr);
  atlas_image_ = VK_NULL_HANDLE;
  if (atlas_memory_) vkFreeMemory(device_, atlas_memory_, nullptr);
  atlas_memory_ = VK_NULL_HANDLE;
  if (ft_lib_) FT_Done_FreeType(ft_lib_);
  ft_lib_ = nullptr;
  device_ = VK_NULL_HANDLE;
  physical_ = VK_NULL_HANDLE;
  pool_ = VK_NULL_HANDLE;
  queue_ = VK_NULL_HANDLE;
}

bool FontManager::build_atlas(Font& f) {
  // Rasterize ASCII 32..126 into a 512x512 atlas (linear, host-visible).
  const int ATLAS = 512;
  // destroy previous atlas
  if (atlas_view_) { vkDestroyImageView(device_, atlas_view_, nullptr); atlas_view_ = VK_NULL_HANDLE; }
  if (atlas_sampler_) { vkDestroySampler(device_, atlas_sampler_, nullptr); atlas_sampler_ = VK_NULL_HANDLE; }
  if (atlas_image_) { vkDestroyImage(device_, atlas_image_, nullptr); atlas_image_ = VK_NULL_HANDLE; }
  if (atlas_memory_) { vkFreeMemory(device_, atlas_memory_, nullptr); atlas_memory_ = VK_NULL_HANDLE; }

  VkImageCreateInfo ici{};
  ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  ici.imageType = VK_IMAGE_TYPE_2D;
  ici.format = VK_FORMAT_R8_UNORM;
  ici.extent = {ATLAS, ATLAS, 1};
  ici.mipLevels = 1;
  ici.arrayLayers = 1;
  ici.samples = VK_SAMPLE_COUNT_1_BIT;
  ici.tiling = VK_IMAGE_TILING_LINEAR;
  ici.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
  ici.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
  if (vkCreateImage(device_, &ici, nullptr, &atlas_image_) != VK_SUCCESS) return false;

  VkMemoryRequirements req{};
  vkGetImageMemoryRequirements(device_, atlas_image_, &req);
  int mt = find_memory_type(physical_, req.memoryTypeBits,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  if (mt < 0) return false;
  VkMemoryAllocateInfo mai{};
  mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  mai.allocationSize = req.size;
  mai.memoryTypeIndex = static_cast<std::uint32_t>(mt);
  if (vkAllocateMemory(device_, &mai, nullptr, &atlas_memory_) != VK_SUCCESS) return false;
  vkBindImageMemory(device_, atlas_image_, atlas_memory_, 0);

  // Map and clear.
  void* mapped = nullptr;
  vkMapMemory(device_, atlas_memory_, 0, VK_WHOLE_SIZE, 0, &mapped);
  std::vector<unsigned char> buf(ATLAS * ATLAS, 0);
  // Rasterize
  int pen_x = 0, pen_y = 0, row_h = 0;
  for (int c = 32; c < 127; ++c) {
    if (FT_Load_Char(f.face, static_cast<FT_ULong>(c), FT_LOAD_RENDER)) continue;
    FT_GlyphSlot g = f.face->glyph;
    if (pen_x + static_cast<int>(g->bitmap.width) + 1 >= ATLAS) {
      pen_x = 0;
      pen_y += row_h + 1;
      row_h = 0;
    }
    if (pen_y + static_cast<int>(g->bitmap.rows) + 1 >= ATLAS) break;
    // copy glyph bitmap into atlas buffer
    for (unsigned int r = 0; r < g->bitmap.rows; ++r) {
      for (unsigned int col = 0; col < g->bitmap.width; ++col) {
        buf[static_cast<std::size_t>((pen_y + static_cast<int>(r)) * ATLAS + (pen_x + static_cast<int>(col)))] =
            g->bitmap.buffer[r * static_cast<unsigned int>(g->bitmap.pitch) + col];
      }
    }
    Glyph gl{};
    gl.width = static_cast<int>(g->bitmap.width);
    gl.height = static_cast<int>(g->bitmap.rows);
    gl.bearing_x = g->bitmap_left;
    gl.bearing_y = g->bitmap_top;
    gl.advance = static_cast<int>(g->advance.x >> 6);
    gl.u0 = static_cast<float>(pen_x) / ATLAS;
    gl.v0 = static_cast<float>(pen_y) / ATLAS;
    gl.u1 = static_cast<float>(pen_x + gl.width) / ATLAS;
    gl.v1 = static_cast<float>(pen_y + gl.height) / ATLAS;
    f.glyphs[static_cast<std::size_t>(c)] = gl;
    pen_x += gl.width + 1;
    row_h = std::max(row_h, gl.height);
  }
  // Upload
  if (mapped) {
    // Need to account for linear image row pitch.
    VkImageSubresource sub{};
    sub.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    VkSubresourceLayout layout{};
    vkGetImageSubresourceLayout(device_, atlas_image_, &sub, &layout);
    for (int y = 0; y < ATLAS; ++y) {
      std::memcpy(static_cast<unsigned char*>(mapped) + layout.offset + static_cast<VkDeviceSize>(y) * layout.rowPitch,
                  buf.data() + y * ATLAS, ATLAS);
    }
    vkUnmapMemory(device_, atlas_memory_);
  }

  // Image view
  VkImageViewCreateInfo vci{};
  vci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  vci.image = atlas_image_;
  vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
  vci.format = VK_FORMAT_R8_UNORM;
  vci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  vci.subresourceRange.baseMipLevel = 0;
  vci.subresourceRange.levelCount = 1;
  vci.subresourceRange.baseArrayLayer = 0;
  vci.subresourceRange.layerCount = 1;
  if (vkCreateImageView(device_, &vci, nullptr, &atlas_view_) != VK_SUCCESS) return false;

  // Sampler
  VkSamplerCreateInfo sci{};
  sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sci.magFilter = VK_FILTER_LINEAR;
  sci.minFilter = VK_FILTER_LINEAR;
  sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  vkCreateSampler(device_, &sci, nullptr, &atlas_sampler_);

  atlas_w_ = ATLAS;
  atlas_h_ = ATLAS;
  return true;
}

int FontManager::load_font(const char* path, int pixel_size) {
  if (!ft_lib_ || !path) return 0;
  Font f;
  f.pixel_size = pixel_size;
  f.glyphs.resize(256);
  if (FT_New_Face(ft_lib_, path, 0, &f.face) != 0) return 0;
  FT_Set_Pixel_Sizes(f.face, 0, static_cast<unsigned int>(pixel_size));
  if (!build_atlas(f)) {
    FT_Done_Face(f.face);
    return 0;
  }
  fonts_.push_back(f);
  return static_cast<int>(fonts_.size()); // 1-based handle
}

bool FontManager::get_glyph(int font_handle, unsigned char c, Glyph& out) const {
  if (font_handle < 1 || font_handle > static_cast<int>(fonts_.size())) return false;
  const Font& f = fonts_[static_cast<std::size_t>(font_handle - 1)];
  if (c >= f.glyphs.size()) return false;
  out = f.glyphs[c];
  return true;
}

}  // namespace fl_port
