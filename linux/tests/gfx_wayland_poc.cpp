// #4 standalone Wayland/SDL3/Vulkan presentation PoC.
// Validates window + Vulkan instance/surface + device/queue selection + swapchain + clear-color present.
// Build requires SDL3 and Vulkan development libraries usable for the target architecture.

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <thread>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

namespace {

void sdl_die(const char* what) {
  std::cerr << "[gfx_poc] " << what << " failed: " << SDL_GetError() << "\n";
}

struct QueueIndices {
  uint32_t graphics = UINT32_MAX;
  uint32_t present = UINT32_MAX;
  bool complete() const { return graphics != UINT32_MAX && present != UINT32_MAX; }
};

QueueIndices find_queues(VkPhysicalDevice gpu, VkSurfaceKHR surface) {
  QueueIndices out;
  uint32_t count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(gpu, &count, nullptr);
  std::vector<VkQueueFamilyProperties> props(count);
  if (count) vkGetPhysicalDeviceQueueFamilyProperties(gpu, &count, props.data());
  for (uint32_t i = 0; i < count; ++i) {
    if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) out.graphics = i;
    VkBool32 present = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface, &present);
    if (present) out.present = i;
    if (out.complete()) break;
  }
  return out;
}

bool has_swapchain_ext(VkPhysicalDevice gpu) {
  uint32_t count = 0;
  if (vkEnumerateDeviceExtensionProperties(gpu, nullptr, &count, nullptr) != VK_SUCCESS) return false;
  std::vector<VkExtensionProperties> exts(count);
  vkEnumerateDeviceExtensionProperties(gpu, nullptr, &count, exts.data());
  for (const auto& e : exts) {
    if (std::strcmp(e.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) return true;
  }
  return false;
}

VkSurfaceFormatKHR choose_surface_format(const std::vector<VkSurfaceFormatKHR>& formats) {
  for (const auto& f : formats) {
    if ((f.format == VK_FORMAT_B8G8R8A8_UNORM || f.format == VK_FORMAT_R8G8B8A8_UNORM) &&
        f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return f;
    }
  }
  return formats.empty() ? VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}
                         : formats[0];
}

VkPresentModeKHR choose_present_mode(const std::vector<VkPresentModeKHR>& modes) {
  for (auto m : modes) if (m == VK_PRESENT_MODE_MAILBOX_KHR) return m;
  return VK_PRESENT_MODE_FIFO_KHR; // guaranteed
}

VkExtent2D choose_extent(const VkSurfaceCapabilitiesKHR& caps, SDL_Window* window) {
  if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) return caps.currentExtent;
  int w = 0, h = 0;
  SDL_GetWindowSizeInPixels(window, &w, &h);
  VkExtent2D e{static_cast<uint32_t>(w > 0 ? w : 960), static_cast<uint32_t>(h > 0 ? h : 540)};
  e.width = std::clamp(e.width, caps.minImageExtent.width, caps.maxImageExtent.width);
  e.height = std::clamp(e.height, caps.minImageExtent.height, caps.maxImageExtent.height);
  return e;
}

} // namespace

int main(int argc, char** argv) {
  (void)argc; (void)argv;
  std::cout << "[gfx_poc] starting (SDL3+Vulkan Wayland presentation probe)\n";

  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    sdl_die("SDL_Init");
    return 2;
  }

  SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "wayland");
  SDL_Window* window = SDL_CreateWindow("FL Linux GFX PoC", 960, 540,
                                        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
  if (!window) {
    std::cerr << "[gfx_poc] Wayland window create failed, retrying default driver: " << SDL_GetError() << "\n";
    SDL_ClearError();
    window = SDL_CreateWindow("FL Linux GFX PoC", 960, 540,
                              SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
  }
  if (!window) {
    sdl_die("SDL_CreateWindow");
    SDL_Quit();
    return 3;
  }

  std::cout << "[gfx_poc] SDL platform: " << SDL_GetPlatform() << "\n";
  std::cout << "[gfx_poc] SDL video driver: " << (SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "<none>") << "\n";

  Uint32 ext_count = 0;
  const char* const* sdl_exts = SDL_Vulkan_GetInstanceExtensions(&ext_count);
  if (!sdl_exts || ext_count == 0) {
    sdl_die("SDL_Vulkan_GetInstanceExtensions");
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 4;
  }

  VkApplicationInfo app{};
  app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app.pApplicationName = "FL Linux GFX PoC";
  app.apiVersion = VK_API_VERSION_1_1;

  VkInstanceCreateInfo ici{};
  ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  ici.pApplicationInfo = &app;
  ici.enabledExtensionCount = ext_count;
  ici.ppEnabledExtensionNames = sdl_exts;

  VkInstance instance = VK_NULL_HANDLE;
  VkResult vr = vkCreateInstance(&ici, nullptr, &instance);
  if (vr != VK_SUCCESS) {
    std::cerr << "[gfx_poc] vkCreateInstance failed: " << vr << "\n";
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 5;
  }

  VkSurfaceKHR surface = VK_NULL_HANDLE;
  if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) {
    sdl_die("SDL_Vulkan_CreateSurface");
    vkDestroyInstance(instance, nullptr);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 6;
  }

  uint32_t gpu_count = 0;
  vkEnumeratePhysicalDevices(instance, &gpu_count, nullptr);
  std::vector<VkPhysicalDevice> gpus(gpu_count);
  if (gpu_count) vkEnumeratePhysicalDevices(instance, &gpu_count, gpus.data());
  std::cout << "[gfx_poc] Vulkan physical devices: " << gpu_count << "\n";
  if (gpus.empty()) {
    std::cerr << "[gfx_poc] no physical device\n";
    SDL_Vulkan_DestroySurface(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 7;
  }

  VkPhysicalDevice gpu = VK_NULL_HANDLE;
  QueueIndices qi;
  for (auto cand : gpus) {
    qi = find_queues(cand, surface);
    if (qi.complete() && has_swapchain_ext(cand)) { gpu = cand; break; }
  }
  if (!gpu) { gpu = gpus[0]; qi = find_queues(gpu, surface); }
  VkPhysicalDeviceProperties props{};
  vkGetPhysicalDeviceProperties(gpu, &props);
  std::cout << "[gfx_poc] selected device: " << props.deviceName << "\n";
  if (!qi.complete() || !has_swapchain_ext(gpu)) {
    std::cerr << "[gfx_poc] device/queue/swapchain support insufficient (scaffold partial)\n";
    SDL_Vulkan_DestroySurface(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 8;
  }

  float prio = 1.0f;
  std::vector<VkDeviceQueueCreateInfo> qcis;
  std::vector<uint32_t> unique_q{qi.graphics};
  if (qi.present != qi.graphics) unique_q.push_back(qi.present);
  for (uint32_t qf : unique_q) {
    VkDeviceQueueCreateInfo qci{};
    qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qci.queueFamilyIndex = qf;
    qci.queueCount = 1;
    qci.pQueuePriorities = &prio;
    qcis.push_back(qci);
  }
  const char* dev_exts[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  VkDeviceCreateInfo dci{};
  dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  dci.queueCreateInfoCount = static_cast<uint32_t>(qcis.size());
  dci.pQueueCreateInfos = qcis.data();
  dci.enabledExtensionCount = 1;
  dci.ppEnabledExtensionNames = dev_exts;

  VkDevice device = VK_NULL_HANDLE;
  vr = vkCreateDevice(gpu, &dci, nullptr, &device);
  if (vr != VK_SUCCESS) {
    std::cerr << "[gfx_poc] vkCreateDevice failed: " << vr << "\n";
    SDL_Vulkan_DestroySurface(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 9;
  }

  VkQueue gfx_q = VK_NULL_HANDLE, pres_q = VK_NULL_HANDLE;
  vkGetDeviceQueue(device, qi.graphics, 0, &gfx_q);
  vkGetDeviceQueue(device, qi.present, 0, &pres_q);

  VkSurfaceCapabilitiesKHR caps{};
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &caps);
  uint32_t fmt_count = 0, pm_count = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &fmt_count, nullptr);
  vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &pm_count, nullptr);
  std::vector<VkSurfaceFormatKHR> formats(fmt_count);
  std::vector<VkPresentModeKHR> modes(pm_count);
  if (fmt_count) vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &fmt_count, formats.data());
  if (pm_count) vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &pm_count, modes.data());
  if (formats.empty() || modes.empty()) {
    std::cerr << "[gfx_poc] swapchain formats/present modes unavailable\n";
    vkDestroyDevice(device, nullptr);
    SDL_Vulkan_DestroySurface(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 10;
  }

  VkSurfaceFormatKHR sf = choose_surface_format(formats);
  VkPresentModeKHR pm = choose_present_mode(modes);
  VkExtent2D extent = choose_extent(caps, window);
  uint32_t image_count = caps.minImageCount + 1;
  if (caps.maxImageCount > 0 && image_count > caps.maxImageCount) image_count = caps.maxImageCount;

  VkSwapchainCreateInfoKHR sci{};
  sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  sci.surface = surface;
  sci.minImageCount = image_count;
  sci.imageFormat = sf.format;
  sci.imageColorSpace = sf.colorSpace;
  sci.imageExtent = extent;
  sci.imageArrayLayers = 1;
  sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  uint32_t qfis[] = {qi.graphics, qi.present};
  if (qi.graphics != qi.present) {
    sci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    sci.queueFamilyIndexCount = 2;
    sci.pQueueFamilyIndices = qfis;
  } else {
    sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }
  sci.preTransform = caps.currentTransform;
  sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  sci.presentMode = pm;
  sci.clipped = VK_TRUE;

  VkSwapchainKHR swapchain = VK_NULL_HANDLE;
  vr = vkCreateSwapchainKHR(device, &sci, nullptr, &swapchain);
  if (vr != VK_SUCCESS) {
    std::cerr << "[gfx_poc] vkCreateSwapchainKHR failed: " << vr << "\n";
    vkDestroyDevice(device, nullptr);
    SDL_Vulkan_DestroySurface(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 11;
  }

  uint32_t sc_count = 0;
  vkGetSwapchainImagesKHR(device, swapchain, &sc_count, nullptr);
  std::vector<VkImage> images(sc_count);
  vkGetSwapchainImagesKHR(device, swapchain, &sc_count, images.data());
  std::cout << "[gfx_poc] swapchain images: " << sc_count << " extent=" << extent.width << "x" << extent.height << "\n";

  VkCommandPoolCreateInfo cpci{};
  cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cpci.queueFamilyIndex = qi.graphics;
  cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  VkCommandPool cmd_pool = VK_NULL_HANDLE;
  vr = vkCreateCommandPool(device, &cpci, nullptr, &cmd_pool);
  if (vr != VK_SUCCESS) {
    std::cerr << "[gfx_poc] vkCreateCommandPool failed: " << vr << "\n";
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyDevice(device, nullptr);
    SDL_Vulkan_DestroySurface(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 12;
  }

  VkCommandBufferAllocateInfo cbai{};
  cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cbai.commandPool = cmd_pool;
  cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cbai.commandBufferCount = 1;
  VkCommandBuffer cmd = VK_NULL_HANDLE;
  vkAllocateCommandBuffers(device, &cbai, &cmd);

  VkSemaphoreCreateInfo sci_sem{};
  sci_sem.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  VkFenceCreateInfo fci{};
  fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  VkSemaphore img_sem = VK_NULL_HANDLE, done_sem = VK_NULL_HANDLE;
  VkFence fence = VK_NULL_HANDLE;
  vkCreateSemaphore(device, &sci_sem, nullptr, &img_sem);
  vkCreateSemaphore(device, &sci_sem, nullptr, &done_sem);
  vkCreateFence(device, &fci, nullptr, &fence);

  auto record_clear = [&](uint32_t image_index, float t) {
    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &bi);

    VkImageMemoryBarrier to_clear{}; 
    to_clear.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    to_clear.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    to_clear.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    to_clear.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    to_clear.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    to_clear.image = images[image_index];
    to_clear.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    to_clear.subresourceRange.baseMipLevel = 0;
    to_clear.subresourceRange.levelCount = 1;
    to_clear.subresourceRange.baseArrayLayer = 0;
    to_clear.subresourceRange.layerCount = 1;
    to_clear.srcAccessMask = 0;
    to_clear.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &to_clear);

    VkClearColorValue clear{};
    clear.float32[0] = 0.05f + 0.20f * t;
    clear.float32[1] = 0.08f + 0.35f * (1.0f - t);
    clear.float32[2] = 0.20f + 0.55f * t;
    clear.float32[3] = 1.0f;
    VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCmdClearColorImage(cmd, images[image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear, 1, &range);

    VkImageMemoryBarrier to_present = to_clear;
    to_present.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    to_present.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    to_present.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    to_present.dstAccessMask = 0;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &to_present);
    vkEndCommandBuffer(cmd);
  };

  std::cout << "[gfx_poc] presenting clear-color frames...\n";
  const int target_frames = 120;
  int presented = 0;
  bool ok = true;
  for (int frame = 0; frame < target_frames; ++frame) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_EVENT_QUIT) frame = target_frames;
    }
    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &fence);

    uint32_t image_index = 0;
    vr = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, img_sem, VK_NULL_HANDLE, &image_index);
    if (vr != VK_SUCCESS && vr != VK_SUBOPTIMAL_KHR) { ok = false; break; }

    vkResetCommandBuffer(cmd, 0);
    record_clear(image_index, (frame % 60) / 59.0f);

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkSubmitInfo si{};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.waitSemaphoreCount = 1; si.pWaitSemaphores = &img_sem; si.pWaitDstStageMask = &wait_stage;
    si.commandBufferCount = 1; si.pCommandBuffers = &cmd;
    si.signalSemaphoreCount = 1; si.pSignalSemaphores = &done_sem;
    vr = vkQueueSubmit(gfx_q, 1, &si, fence);
    if (vr != VK_SUCCESS) { ok = false; break; }

    VkPresentInfoKHR pi{};
    pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.waitSemaphoreCount = 1; pi.pWaitSemaphores = &done_sem;
    pi.swapchainCount = 1; pi.pSwapchains = &swapchain; pi.pImageIndices = &image_index;
    vr = vkQueuePresentKHR(pres_q, &pi);
    if (vr != VK_SUCCESS && vr != VK_SUBOPTIMAL_KHR) { ok = false; break; }
    ++presented;
  }
  vkDeviceWaitIdle(device);
  std::cout << "[gfx_poc] presented frames: " << presented << "/" << target_frames << (ok ? " (ok)" : " (errors)") << "\n";

  vkDestroyFence(device, fence, nullptr);
  vkDestroySemaphore(device, done_sem, nullptr);
  vkDestroySemaphore(device, img_sem, nullptr);
  vkDestroyCommandPool(device, cmd_pool, nullptr);
  vkDestroySwapchainKHR(device, swapchain, nullptr);
  vkDestroyDevice(device, nullptr);
  SDL_Vulkan_DestroySurface(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);
  SDL_DestroyWindow(window);
  SDL_Quit();
  std::cout << "[gfx_poc] complete\n";
  return ok ? 0 : 13;
}
