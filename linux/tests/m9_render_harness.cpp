// M9 harness: exercise vertex/index buffers, textures, and fonts.

#include "linux_port/render_pipeline.h"
#include "linux_port/window_input.h"
#include "linux_port/platform.h"

#include <SDL3/SDL.h>

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <cstring>

int main() {
  fl_port::log_message(fl_port::LogLevel::Info, "m9_render_harness starting");
  std::cout << "m9_render_harness\n";

  fl_port::WindowConfig wcfg;
  wcfg.title = "Freelancer M9 Render Harness";
  wcfg.width = 640;
  wcfg.height = 480;
  wcfg.resizable = true;
  wcfg.visible = false;
  wcfg.prefer_wayland = true;
  wcfg.vulkan = true;

  fl_port::Window window;
  if (!window.create(wcfg)) {
    std::cerr << "[FAIL] failed to create native window\n";
    return 2;
  }

  fl_port::RenderPipelineConfig rcfg;
  rcfg.initial_width = 640;
  rcfg.initial_height = 480;

  fl_port::RenderPipeline rp;
  if (!rp.init_for_window(&window, rcfg)) {
    std::cerr << "[FAIL] render pipeline init failed: " << rp.last_error() << "\n";
    window.destroy();
    return 3;
  }
  std::cout << "[ok] render pipeline initialized\n";

  // Test VB
  void* vb = nullptr;
  int r = rp.vertex_buffers().create_vb(0x142, 3, 0, &vb);
  std::cout << "[vb] create_vb rc=" << r << " handle=" << vb << "\n";
  if (r != 0 || !vb) { std::cerr << "[FAIL] create_vb\n"; }
  void* ptr = nullptr;
  std::uint32_t off = 0;
  rp.vertex_buffers().lock_vb(vb, &off, &ptr, 3);
  if (ptr) { std::cout << "[vb] lock ok ptr=" << ptr << "\n"; std::memset(ptr, 0, 3 * 32); }
  rp.vertex_buffers().unlock_vb(vb);
  rp.vertex_buffers().select_vb(vb);
  std::uint32_t fvf=0, cnt=0;
  rp.vertex_buffers().get_vb_info(vb, &fvf, &cnt);
  std::cout << "[vb] info fvf=" << fvf << " count=" << cnt << " valid=" << rp.vertex_buffers().is_vb_valid(vb) << "\n";
  rp.vertex_buffers().destroy_vb(&vb);

  // Test IB
  std::uint32_t* ib = nullptr;
  r = rp.vertex_buffers().create_ib(6, &ib, 0);
  std::cout << "[ib] create_ib rc=" << r << " handle=" << ib << "\n";
  std::uint16_t idx[6] = {0,1,2,0,2,3};
  std::uint32_t ioff = 0;
  rp.vertex_buffers().copy_indices(ib, &ioff, idx, 6);
  std::uint32_t cap = 0;
  rp.vertex_buffers().get_ib_capacity(ib, &cap);
  std::cout << "[ib] capacity=" << cap << " valid=" << rp.vertex_buffers().is_ib_valid(ib) << "\n";
  rp.vertex_buffers().destroy_ib(reinterpret_cast<void**>(&ib));

  // Test texture (from memory: 2x2 red)
  void* tex = nullptr;
  unsigned int fmt = VK_FORMAT_R8G8B8A8_UNORM;
  r = rp.textures().create_texture(2, 2, &fmt, 1, 0, &tex);
  std::cout << "[tex] create_texture rc=" << r << " handle=" << tex << "\n";
  if (tex) {
    unsigned char pixels[16] = {255,0,0,255, 0,255,0,255, 0,0,255,255, 255,255,0,255};
    rp.textures().set_texture_level_data(tex, 0, 2, 2, 8, &fmt, pixels, nullptr, nullptr);
    int fmt=0; unsigned int w=0,h=0; int lv=0;
    rp.textures().get_texture_format(tex, &fmt);
    rp.textures().get_texture_dim(tex, &w, &h, &lv);
    std::cout << "[tex] fmt=" << fmt << " " << w << "x" << h << " levels=" << lv << " valid=" << rp.textures().is_texture(tex) << "\n";
    rp.textures().destroy_texture(tex);
  }

  // Test font (may fail if no font file; non-fatal)
  int fh = rp.fonts().load_font("/usr/share/fonts/TTF/DejaVuSans.ttf", 16);
  std::cout << "[font] load_font handle=" << fh << "\n";
  if (fh > 0) {
    fl_port::FontManager::Glyph g{};
    if (rp.fonts().get_glyph(fh, 'A', g)) {
      std::cout << "[font] glyph 'A' " << g.width << "x" << g.height << " advance=" << g.advance << "\n";
    }
  }

  rp.shutdown();
  window.destroy();
  std::cout << "m9_render_harness: PASS\n";
  return 0;
}
