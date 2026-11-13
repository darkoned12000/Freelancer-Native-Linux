#pragma once

// M9.4: FreeType-based font rendering for the Linux port.

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace fl_port {

class FontManager {
 public:
  struct Glyph {
    int width = 0;
    int height = 0;
    int bearing_x = 0;
    int bearing_y = 0;
    int advance = 0;
    float u0 = 0, v0 = 0, u1 = 0, v1 = 0; // atlas UVs
  };

  FontManager() = default;
  ~FontManager();

  FontManager(const FontManager&) = delete;
  FontManager& operator=(const FontManager&) = delete;

  bool init(VkDevice device, VkPhysicalDevice physical, VkCommandPool pool, VkQueue queue);
  void shutdown();
  bool is_initialized() const { return ft_lib_ != nullptr; }

  // Load a font file at the given pixel size. Returns a handle (index+1) or 0 on failure.
  int load_font(const char* path, int pixel_size);
  bool get_glyph(int font_handle, unsigned char c, Glyph& out) const;

  // Atlas texture (Vulkan image view) for the last loaded font, for binding.
  VkImageView atlas_view() const { return atlas_view_; }
  VkSampler atlas_sampler() const { return atlas_sampler_; }
  int atlas_width() const { return atlas_w_; }
  int atlas_height() const { return atlas_h_; }

 private:
  struct Font {
    FT_Face face = nullptr;
    int pixel_size = 0;
    int atlas_x = 0, atlas_y = 0, atlas_row_h = 0;
    std::vector<Glyph> glyphs; // indexed by char (256)
  };

  bool build_atlas(Font& f);
  FT_Library ft_lib_ = nullptr;
  VkDevice device_ = VK_NULL_HANDLE;
  VkPhysicalDevice physical_ = VK_NULL_HANDLE;
  VkCommandPool pool_ = VK_NULL_HANDLE;
  VkQueue queue_ = VK_NULL_HANDLE;
  std::vector<Font> fonts_;
  // Atlas image (shared, rebuilt per font load for simplicity).
  VkImage atlas_image_ = VK_NULL_HANDLE;
  VkDeviceMemory atlas_memory_ = VK_NULL_HANDLE;
  VkImageView atlas_view_ = VK_NULL_HANDLE;
  VkSampler atlas_sampler_ = VK_NULL_HANDLE;
  int atlas_w_ = 0, atlas_h_ = 0;
};

}  // namespace fl_port
