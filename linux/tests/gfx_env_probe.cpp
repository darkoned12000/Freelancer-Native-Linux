// #4 prerequisite validation probe (32-bit preferred in build-x86).
// Reports availability of SDL3/Wayland/Vulkan build/runtime signals detectable at compile/runtime.

#include <iostream>

#if defined(FL_HAS_SDL3)
#include <SDL3/SDL.h>
#endif
#if defined(FL_HAS_VULKAN)
#include <vulkan/vulkan.h>
#endif

int main() {
  std::cout << "gfx_env_probe\n";
#if defined(FL_HAS_SDL3)
  std::cout << "SDL3 compile: yes\n";
  if (SDL_Init(0)) {
    std::cout << "SDL_Init(0): ok\n";
    std::cout << "SDL_GetPlatform(): " << SDL_GetPlatform() << "\n";
    SDL_Quit();
  } else {
    std::cout << "SDL_Init(0): fail: " << SDL_GetError() << "\n";
  }
#else
  std::cout << "SDL3 compile: no\n";
#endif

#if defined(FL_HAS_VULKAN)
  std::cout << "Vulkan headers: yes\n";
  uint32_t count = 0;
  VkResult r = vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
  std::cout << "vkEnumerateInstanceExtensionProperties: " << (r == VK_SUCCESS ? "ok" : "fail")
            << " count=" << count << "\n";
#else
  std::cout << "Vulkan headers: no\n";
#endif

#if defined(__i386__) || defined(__i386) || defined(i386)
  std::cout << "arch: i386 (32-bit)\n";
#elif defined(__x86_64__) || defined(_M_X64)
  std::cout << "arch: x86_64\n";
#else
  std::cout << "arch: other\n";
#endif
  return 0;
}
