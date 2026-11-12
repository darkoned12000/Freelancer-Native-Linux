// M5: scoped native Vulkan backend implementing an IRenderPipeline-style subset.
// Wired to the M4 Window for surface creation. Clears and presents a frame,
// proving a native in-engine frame on Linux (Wayland-first). (Draw submission
// is a stub; vertex buffer/pipeline draw is a follow-on.)

#include "linux_port/render_pipeline.h"
#include "linux_port/window_input.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <vulkan/vulkan.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace fl_port {

namespace {

void set_identity_4x4(float* m) {
  for (int i = 0; i < 16; ++i) m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
}
void set_identity_3x4(float* m) {
  // 3x4 row-major identity (12 floats).
  for (int i = 0; i < 12; ++i) m[i] = 0.0f;
  m[0] = m[5] = m[10] = 1.0f;
}

const char* vk_result_str(VkResult r) {
  switch (r) {
    case VK_SUCCESS: return "VK_SUCCESS";
    case VK_NOT_READY: return "VK_NOT_READY";
    case VK_TIMEOUT: return "VK_TIMEOUT";
    case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
    default: return "VkResult";
  }
}

}  // namespace

struct RenderPipeline::VkState {
  VkInstance instance = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  VkPhysicalDevice gpu = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;
  VkQueue graphics_queue = VK_NULL_HANDLE;
  VkQueue present_queue = VK_NULL_HANDLE;
  uint32_t graphics_family = 0;
  uint32_t present_family = 0;

  VkSwapchainKHR swapchain = VK_NULL_HANDLE;
  VkFormat swapchain_format = VK_FORMAT_B8G8R8A8_UNORM;
  VkColorSpaceKHR swapchain_colorspace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  VkExtent2D swapchain_extent{};
  std::vector<VkImage> swapchain_images;
  std::vector<VkImageView> swapchain_image_views;

  VkRenderPass render_pass = VK_NULL_HANDLE;
  std::vector<VkFramebuffer> framebuffers;

  VkCommandPool command_pool = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer> command_buffers;

  VkSemaphore image_available = VK_NULL_HANDLE;
  std::vector<VkSemaphore> render_finished;
  VkFence in_flight = VK_NULL_HANDLE;

  uint32_t current_image = 0;
};

RenderPipeline::RenderPipeline() {
  identity_matrices();
}

RenderPipeline::~RenderPipeline() { shutdown(); }

void RenderPipeline::set_error(const std::string& msg) {
  last_error_ = msg;
  std::cerr << "[fl-port][render] " << msg << "\n";
}

void RenderPipeline::identity_matrices() {
  set_identity_3x4(world_);
  set_identity_3x4(view_);
  set_identity_3x4(modelview_);
  set_identity_4x4(projection_);
  viewport_[0] = viewport_[1] = viewport_[2] = viewport_[3] = 0;
  depth_range_[0] = 0.0f;
  depth_range_[1] = 1.0f;
}

// ---------------------------------------------------------------------------
// IRenderPipeline (minimal subset)
// ---------------------------------------------------------------------------

GENRESULT RenderPipeline::Startup(const char* /*SectionName*/) {
  // Vulkan init is deferred to CreateBuffers (needs window handle).
  return GR_OK;
}

GENRESULT RenderPipeline::ShutdownPipeline() {
  shutdown();
  return GR_OK;
}

GENRESULT RenderPipeline::SetPipelineState(int Id, int Value) {
  if (Id >= 0 && Id < 64) pipeline_state_[Id] = Value;
  return GR_OK;
}
GENRESULT RenderPipeline::GetPipelineState(int Id, int* Out) {
  if (!Out) return GR_INVALID_PARMS;
  if (Id < 0 || Id >= 64) return GR_INVALID_PARMS;
  *Out = pipeline_state_[Id];
  return GR_OK;
}

GENRESULT RenderPipeline::GetDeviceInfo(void* /*Out520*/) { return GR_GENERIC; }
GENRESULT RenderPipeline::QueryDeviceAbility(unsigned int /*Index*/, unsigned long* /*Out*/) { return GR_GENERIC; }
GENRESULT RenderPipeline::GetDisplayModeCount(unsigned int* Out) {
  if (!Out) return GR_INVALID_PARMS;
  *Out = 1;
  return GR_OK;
}
GENRESULT RenderPipeline::GetDisplayModeEnum(unsigned int /*Index*/, void* /*Out*/) { return GR_GENERIC; }
GENRESULT RenderPipeline::SelectMode(int* /*Mode*/, unsigned int* /*OutIndex*/) { return GR_OK; }

GENRESULT RenderPipeline::CreateBuffers(HWND Hwnd, int* /*Mode*/, int* /*OutMode*/) {
  // Hwnd is the opaque adapter handle (SDL_Window*) from M4.
  SDL_Window* sdl_window = reinterpret_cast<SDL_Window*>(Hwnd);
  if (!sdl_window) {
    set_error("CreateBuffers: null window handle");
    return GR_INVALID_PARMS;
  }
  // Store window association.
  tracked_hwnd_ = Hwnd;
  int w = 0, h = 0;
  SDL_GetWindowSizeInPixels(sdl_window, &w, &h);
  tracked_w_ = w; tracked_h_ = h;

  // Reset any partial state from a previous failed attempt.
  if (vk_) shutdown();

  // Initialize Vulkan from the SDL window directly.
  vk_ = new VkState{};
  // Instance
  Uint32 ext_count = 0;
  const char* const* sdl_exts = SDL_Vulkan_GetInstanceExtensions(&ext_count);
  if (!sdl_exts || ext_count == 0) {
    set_error("CreateBuffers: SDL_Vulkan_GetInstanceExtensions failed");
    shutdown();
    return GR_GENERIC;
  }
  VkApplicationInfo app{};
  app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app.pApplicationName = "Freelancer Native Linux";
  app.apiVersion = VK_API_VERSION_1_1;
  VkInstanceCreateInfo ici{};
  ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  ici.pApplicationInfo = &app;
  ici.enabledExtensionCount = ext_count;
  ici.ppEnabledExtensionNames = sdl_exts;
  VkResult vr = vkCreateInstance(&ici, nullptr, &vk_->instance);
  if (vr != VK_SUCCESS) {
    set_error(std::string("CreateBuffers: vkCreateInstance failed ") + vk_result_str(vr));
    shutdown();
    return GR_GENERIC;
  }
  if (!SDL_Vulkan_CreateSurface(sdl_window, vk_->instance, nullptr, &vk_->surface)) {
    set_error("CreateBuffers: SDL_Vulkan_CreateSurface failed");
    vkDestroyInstance(vk_->instance, nullptr);
    shutdown();
    return GR_GENERIC;
  }
  // Pick GPU
  uint32_t gpu_count = 0;
  vkEnumeratePhysicalDevices(vk_->instance, &gpu_count, nullptr);
  std::vector<VkPhysicalDevice> gpus(gpu_count);
  if (gpu_count) vkEnumeratePhysicalDevices(vk_->instance, &gpu_count, gpus.data());
  if (gpus.empty()) {
    set_error("CreateBuffers: no Vulkan physical device");
    SDL_Vulkan_DestroySurface(vk_->instance, vk_->surface, nullptr);
    vkDestroyInstance(vk_->instance, nullptr);
    shutdown();
    return GR_GENERIC;
  }
  // Find a GPU with graphics+present+swapchain.
  for (auto cand : gpus) {
    uint32_t qf_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(cand, &qf_count, nullptr);
    std::vector<VkQueueFamilyProperties> qprops(qf_count);
    if (qf_count) vkGetPhysicalDeviceQueueFamilyProperties(cand, &qf_count, qprops.data());
    uint32_t gfx = UINT32_MAX, pres = UINT32_MAX;
    for (uint32_t i = 0; i < qf_count; ++i) {
      if (qprops[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) gfx = i;
      VkBool32 p = VK_FALSE;
      vkGetPhysicalDeviceSurfaceSupportKHR(cand, i, vk_->surface, &p);
      if (p) pres = i;
      if (gfx != UINT32_MAX && pres != UINT32_MAX) break;
    }
    // swapchain ext check
    uint32_t extc = 0;
    vkEnumerateDeviceExtensionProperties(cand, nullptr, &extc, nullptr);
    std::vector<VkExtensionProperties> exts(extc);
    if (extc) vkEnumerateDeviceExtensionProperties(cand, nullptr, &extc, exts.data());
    bool has_sc = false;
    for (auto& e : exts) if (std::strcmp(e.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) has_sc = true;
    if (gfx != UINT32_MAX && pres != UINT32_MAX && has_sc) {
      vk_->gpu = cand;
      vk_->graphics_family = gfx;
      vk_->present_family = pres;
      break;
    }
  }
  if (vk_->gpu == VK_NULL_HANDLE) {
    set_error("CreateBuffers: no suitable GPU");
    SDL_Vulkan_DestroySurface(vk_->instance, vk_->surface, nullptr);
    vkDestroyInstance(vk_->instance, nullptr);
    shutdown();
    return GR_GENERIC;
  }
  // Create device
  float prio = 1.0f;
  std::vector<VkDeviceQueueCreateInfo> qcis;
  std::vector<uint32_t> uniq;
  uniq.push_back(vk_->graphics_family);
  if (vk_->present_family != vk_->graphics_family) uniq.push_back(vk_->present_family);
  for (uint32_t qf : uniq) {
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
  vr = vkCreateDevice(vk_->gpu, &dci, nullptr, &vk_->device);
  if (vr != VK_SUCCESS) {
    set_error(std::string("CreateBuffers: vkCreateDevice failed ") + vk_result_str(vr));
    SDL_Vulkan_DestroySurface(vk_->instance, vk_->surface, nullptr);
    vkDestroyInstance(vk_->instance, nullptr);
    shutdown();
    return GR_GENERIC;
  }
  vkGetDeviceQueue(vk_->device, vk_->graphics_family, 0, &vk_->graphics_queue);
  vkGetDeviceQueue(vk_->device, vk_->present_family, 0, &vk_->present_queue);

  // Surface format
  uint32_t fmt_count = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(vk_->gpu, vk_->surface, &fmt_count, nullptr);
  std::vector<VkSurfaceFormatKHR> fmts(fmt_count);
  if (fmt_count) vkGetPhysicalDeviceSurfaceFormatsKHR(vk_->gpu, vk_->surface, &fmt_count, fmts.data());
  for (auto& f : fmts) {
    if ((f.format == VK_FORMAT_B8G8R8A8_UNORM || f.format == VK_FORMAT_R8G8B8A8_UNORM) &&
        f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      vk_->swapchain_format = f.format;
      vk_->swapchain_colorspace = f.colorSpace;
      break;
    }
  }
  if (fmts.empty()) {
    vk_->swapchain_format = VK_FORMAT_B8G8R8A8_UNORM;
    vk_->swapchain_colorspace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  } else if (vk_->swapchain_format == VK_FORMAT_B8G8R8A8_UNORM && fmts[0].format != VK_FORMAT_B8G8R8A8_UNORM) {
    vk_->swapchain_format = fmts[0].format;
    vk_->swapchain_colorspace = fmts[0].colorSpace;
  }

  // Swapchain extent
  VkSurfaceCapabilitiesKHR caps{};
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_->gpu, vk_->surface, &caps);
  if (caps.currentExtent.width != UINT32_MAX) {
    vk_->swapchain_extent = caps.currentExtent;
  } else {
    vk_->swapchain_extent = {static_cast<uint32_t>(w > 0 ? w : 960),
                            static_cast<uint32_t>(h > 0 ? h : 540)};
    // Clamp to supported extent range (Wayland may report 0-sized when minimized).
    if (vk_->swapchain_extent.width < caps.minImageExtent.width) vk_->swapchain_extent.width = caps.minImageExtent.width;
    if (vk_->swapchain_extent.height < caps.minImageExtent.height) vk_->swapchain_extent.height = caps.minImageExtent.height;
    if (caps.maxImageExtent.width > 0 && vk_->swapchain_extent.width > caps.maxImageExtent.width) vk_->swapchain_extent.width = caps.maxImageExtent.width;
    if (caps.maxImageExtent.height > 0 && vk_->swapchain_extent.height > caps.maxImageExtent.height) vk_->swapchain_extent.height = caps.maxImageExtent.height;
  }

  // Create swapchain
  uint32_t img_count = caps.minImageCount + 1;
  if (caps.maxImageCount > 0 && img_count > caps.maxImageCount) img_count = caps.maxImageCount;
  VkSwapchainCreateInfoKHR sci{};
  sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  sci.surface = vk_->surface;
  sci.minImageCount = img_count;
  sci.imageFormat = vk_->swapchain_format;
  sci.imageColorSpace = vk_->swapchain_colorspace;
  sci.imageExtent = vk_->swapchain_extent;
  sci.imageArrayLayers = 1;
  sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  if (vk_->graphics_family != vk_->present_family) {
    sci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    uint32_t qf[2] = {vk_->graphics_family, vk_->present_family};
    sci.queueFamilyIndexCount = 2;
    sci.pQueueFamilyIndices = qf;
  } else {
    sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sci.queueFamilyIndexCount = 0;
    sci.pQueueFamilyIndices = nullptr;
  }
  sci.preTransform = caps.currentTransform;
  sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  sci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  sci.clipped = VK_TRUE;
  vr = vkCreateSwapchainKHR(vk_->device, &sci, nullptr, &vk_->swapchain);
  if (vr != VK_SUCCESS) {
    set_error(std::string("CreateBuffers: vkCreateSwapchainKHR failed ") + vk_result_str(vr));
    // cleanup device/instance
    vkDestroyDevice(vk_->device, nullptr);
    SDL_Vulkan_DestroySurface(vk_->instance, vk_->surface, nullptr);
    vkDestroyInstance(vk_->instance, nullptr);
    shutdown();
    return GR_GENERIC;
  }
  // Swapchain images
  uint32_t sc_img_count = 0;
  vkGetSwapchainImagesKHR(vk_->device, vk_->swapchain, &sc_img_count, nullptr);
  vk_->swapchain_images.resize(sc_img_count);
  vkGetSwapchainImagesKHR(vk_->device, vk_->swapchain, &sc_img_count, vk_->swapchain_images.data());

  // Image views
  vk_->swapchain_image_views.resize(sc_img_count);
  for (uint32_t i = 0; i < sc_img_count; ++i) {
    VkImageViewCreateInfo ivci{};
    ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.image = vk_->swapchain_images[i];
    ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format = vk_->swapchain_format;
    ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ivci.subresourceRange.baseMipLevel = 0;
    ivci.subresourceRange.levelCount = 1;
    ivci.subresourceRange.baseArrayLayer = 0;
    ivci.subresourceRange.layerCount = 1;
    vr = vkCreateImageView(vk_->device, &ivci, nullptr, &vk_->swapchain_image_views[i]);
    if (vr != VK_SUCCESS) {
      set_error("CreateBuffers: vkCreateImageView failed");
      shutdown();
      return GR_GENERIC;
    }
  }

  // Render pass (single color attachment, clear+store)
  VkAttachmentDescription color_attach{};
  color_attach.format = vk_->swapchain_format;
  color_attach.samples = VK_SAMPLE_COUNT_1_BIT;
  color_attach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attach.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attach.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attach.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_ref{};
  color_ref.attachment = 0;
  color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_ref;

  VkRenderPassCreateInfo rpci{};
  rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  rpci.attachmentCount = 1;
  rpci.pAttachments = &color_attach;
  rpci.subpassCount = 1;
  rpci.pSubpasses = &subpass;
  vr = vkCreateRenderPass(vk_->device, &rpci, nullptr, &vk_->render_pass);
  if (vr != VK_SUCCESS) {
    set_error("CreateBuffers: vkCreateRenderPass failed");
    shutdown();
    return GR_GENERIC;
  }

  // Framebuffers
  vk_->framebuffers.resize(sc_img_count);
  for (uint32_t i = 0; i < sc_img_count; ++i) {
    VkFramebufferCreateInfo fci{};
    fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fci.renderPass = vk_->render_pass;
    fci.attachmentCount = 1;
    fci.pAttachments = &vk_->swapchain_image_views[i];
    fci.width = vk_->swapchain_extent.width;
    fci.height = vk_->swapchain_extent.height;
    fci.layers = 1;
    vr = vkCreateFramebuffer(vk_->device, &fci, nullptr, &vk_->framebuffers[i]);
    if (vr != VK_SUCCESS) {
      set_error("CreateBuffers: vkCreateFramebuffer failed");
      shutdown();
      return GR_GENERIC;
    }
  }

  // Command pool + buffers
  VkCommandPoolCreateInfo cpci{};
  cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cpci.queueFamilyIndex = vk_->graphics_family;
  cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  vr = vkCreateCommandPool(vk_->device, &cpci, nullptr, &vk_->command_pool);
  if (vr != VK_SUCCESS) {
    set_error("CreateBuffers: vkCreateCommandPool failed");
    shutdown();
    return GR_GENERIC;
  }
  vk_->command_buffers.resize(sc_img_count);
  VkCommandBufferAllocateInfo cbai{};
  cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cbai.commandPool = vk_->command_pool;
  cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cbai.commandBufferCount = sc_img_count;
  vr = vkAllocateCommandBuffers(vk_->device, &cbai, vk_->command_buffers.data());
  if (vr != VK_SUCCESS) {
    set_error("CreateBuffers: vkAllocateCommandBuffers failed");
    shutdown();
    return GR_GENERIC;
  }

  // Sync primitives
  VkSemaphoreCreateInfo sci_sem{};
  sci_sem.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  VkFenceCreateInfo fci_fence{};
  fci_fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fci_fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  vkCreateSemaphore(vk_->device, &sci_sem, nullptr, &vk_->image_available);
  vk_->render_finished.assign(sc_img_count, VK_NULL_HANDLE);
  for (uint32_t i = 0; i < sc_img_count; ++i) {
    vkCreateSemaphore(vk_->device, &sci_sem, nullptr, &vk_->render_finished[i]);
  }
  vkCreateFence(vk_->device, &fci_fence, nullptr, &vk_->in_flight);

  initialized_ = true;
  return GR_OK;
}

GENRESULT RenderPipeline::GetModeBlob(void* /*OutAdapter*/, void* /*OutMode10Dwords*/) { return GR_GENERIC; }
GENRESULT RenderPipeline::DestroyGpuResources() { shutdown(); return GR_OK; }

GENRESULT RenderPipeline::ClearBuffers(unsigned long Flags, const void* /*Rects*/) {
  // Clear is performed in render_frame via render pass loadOp; store flags for potential use.
  (void)Flags;
  return GR_OK;
}

GENRESULT RenderPipeline::SwapBuffers() {
  // Presentation handled by render_frame; this is a no-op compat shim.
  return GR_OK;
}

GENRESULT RenderPipeline::LockBuffer(void* /*Out*/) { return GR_GENERIC; }
GENRESULT RenderPipeline::UnlockBuffer() { return GR_GENERIC; }
GENRESULT RenderPipeline::GetBufferInterface(void* /*Out*/, void* /*Unused*/) { return GR_GENERIC; }
GENRESULT RenderPipeline::GetDeviceStats(void* /*Out*/) { return GR_GENERIC; }

GENRESULT RenderPipeline::SetViewportRect(int X, int Y, int W, int H) {
  viewport_[0] = X; viewport_[1] = Y; viewport_[2] = W; viewport_[3] = H;
  return GR_OK;
}
GENRESULT RenderPipeline::GetViewportRect(int* OutX, int* OutY, int* OutW, int* OutH) {
  if (!OutX || !OutY || !OutW || !OutH) return GR_INVALID_PARMS;
  *OutX = viewport_[0]; *OutY = viewport_[1]; *OutW = viewport_[2]; *OutH = viewport_[3];
  return GR_OK;
}
GENRESULT RenderPipeline::SetDepthRange(float MinZ, float MaxZ) {
  depth_range_[0] = MinZ; depth_range_[1] = MaxZ; return GR_OK;
}
GENRESULT RenderPipeline::GetDepthRange(float* OutMinZ, float* OutMaxZ) {
  if (!OutMinZ || !OutMaxZ) return GR_INVALID_PARMS;
  *OutMinZ = depth_range_[0]; *OutMaxZ = depth_range_[1]; return GR_OK;
}

GENRESULT RenderPipeline::SetWindow(HWND Hwnd, int X, int Y, int W, int H) {
  tracked_hwnd_ = Hwnd;
  tracked_x_ = X; tracked_y_ = Y; tracked_w_ = W; tracked_h_ = H;
  return GR_OK;
}
GENRESULT RenderPipeline::GetWindow(HWND* OutHwnd, int* OutLeft, int* OutRight, int* OutW, int* OutH) {
  if (!OutHwnd) return GR_INVALID_PARMS;
  *OutHwnd = tracked_hwnd_;
  if (OutLeft) *OutLeft = tracked_x_;
  if (OutRight) *OutRight = tracked_x_ + tracked_w_;
  if (OutW) *OutW = tracked_w_;
  if (OutH) *OutH = tracked_h_;
  return GR_OK;
}

GENRESULT RenderPipeline::SetWorld(const float* M12) {
  if (M12) std::memcpy(world_, M12, sizeof(world_)); return GR_OK;
}
GENRESULT RenderPipeline::GetWorld(float* Out12) {
  if (!Out12) return GR_INVALID_PARMS;
  std::memcpy(Out12, world_, sizeof(world_)); return GR_OK;
}
GENRESULT RenderPipeline::SetView(const float* M12) {
  if (M12) std::memcpy(view_, M12, sizeof(view_)); return GR_OK;
}
GENRESULT RenderPipeline::GetView(float* Out12) {
  if (!Out12) return GR_INVALID_PARMS;
  std::memcpy(Out12, view_, sizeof(view_)); return GR_OK;
}
GENRESULT RenderPipeline::SetModelview(const float* M12) {
  if (M12) std::memcpy(modelview_, M12, sizeof(modelview_)); return GR_OK;
}
GENRESULT RenderPipeline::GetModelview(float* Out12) {
  if (!Out12) return GR_INVALID_PARMS;
  std::memcpy(Out12, modelview_, sizeof(modelview_)); return GR_OK;
}
GENRESULT RenderPipeline::SetProjection(const float* M16) {
  if (M16) std::memcpy(projection_, M16, sizeof(projection_)); return GR_OK;
}
GENRESULT RenderPipeline::GetProjection(float* Out16) {
  if (!Out16) return GR_INVALID_PARMS;
  std::memcpy(Out16, projection_, sizeof(projection_)); return GR_OK;
}
GENRESULT RenderPipeline::SetLookat(float EyeX, float EyeY, float EyeZ, float AtX, float AtY,
                                    float AtZ, float UpX, float UpY, float UpZ) {
  // D3D8 LookAtLH.
  float zx = AtX - EyeX, zy = AtY - EyeY, zz = AtZ - EyeZ;
  float zl = std::sqrt(zx*zx + zy*zy + zz*zz);
  if (zl > 0.0f) { zx /= zl; zy /= zl; zz /= zl; }
  // xaxis = normalize(cross(Up, zaxis))
  float xx = UpY*zz - UpZ*zy;
  float xy = UpZ*zx - UpX*zz;
  float xz = UpX*zy - UpY*zx;
  float xl = std::sqrt(xx*xx + xy*xy + xz*xz);
  if (xl > 0.0f) { xx /= xl; xy /= xl; xz /= xl; }
  // yaxis = cross(zaxis, xaxis)
  float yx = zy*xz - zz*xy;
  float yy = zz*xx - zx*xz;
  float yz = zx*xy - zy*xx;
  // view_ is 3x4 row-major: rows xaxis, yaxis, zaxis, translation = -dot(axis, eye)
  view_[0]=xx; view_[1]=xy; view_[2]=xz;
  view_[3]=-(xx*EyeX + xy*EyeY + xz*EyeZ);
  view_[4]=yx; view_[5]=yy; view_[6]=yz;
  view_[7]=-(yx*EyeX + yy*EyeY + yz*EyeZ);
  view_[8]=zx; view_[9]=zy; view_[10]=zz;
  view_[11]=-(zx*EyeX + zy*EyeY + zz*EyeZ);
  return GR_OK;
}
GENRESULT RenderPipeline::SetOrtho(float Left, float Right, float Bottom, float Top,
                                   float NearPlane, float FarPlane) {
  // D3D8 OrthoOffCenterLH.
  for (int i = 0; i < 16; ++i) projection_[i] = 0.0f;
  projection_[0] = 2.0f / (Right - Left);
  projection_[5] = 2.0f / (Top - Bottom);
  projection_[10] = 1.0f / (FarPlane - NearPlane);
  projection_[12] = (Left + Right) / (Left - Right);
  projection_[13] = (Top + Bottom) / (Bottom - Top);
  projection_[14] = NearPlane / (NearPlane - FarPlane);
  projection_[15] = 1.0f;
  return GR_OK;
}
GENRESULT RenderPipeline::SetPerspective(float FovDeg, float Aspect, float NearPlane, float FarPlane) {
  // Minimal perspective projection (right-handed, Vulkan depth [0,1]).
  float f = 1.0f / std::tan(FovDeg * 3.14159265f / 180.0f * 0.5f);
  for (int i = 0; i < 16; ++i) projection_[i] = 0.0f;
  projection_[0] = f / Aspect;
  projection_[5] = f;
  projection_[10] = FarPlane / (FarPlane - NearPlane);
  projection_[11] = 1.0f;
  projection_[14] = -(FarPlane * NearPlane) / (FarPlane - NearPlane);
  return GR_OK;
}

GENRESULT RenderPipeline::SetLight(void* /*Handle*/, const D3DLIGHT8* /*Light*/) { return GR_GENERIC; }
GENRESULT RenderPipeline::DestroyLight(void* /*Handle*/) { return GR_GENERIC; }
GENRESULT RenderPipeline::GetLight(void* /*Handle*/, D3DLIGHT8* /*OutLight*/) { return GR_GENERIC; }
GENRESULT RenderPipeline::SetLightEnable(void* /*Handle*/, int /*Enable*/) { return GR_GENERIC; }
GENRESULT RenderPipeline::GetLightEnable(void* /*Handle*/, int* /*OutEnable*/) { return GR_GENERIC; }
GENRESULT RenderPipeline::SetMaterial(const D3DMATERIAL8* /*Material*/) { return GR_OK; }
GENRESULT RenderPipeline::GetMaterial(D3DMATERIAL8* /*Material*/) { return GR_GENERIC; }

GENRESULT RenderPipeline::CreateTexture(int /*Width*/, int /*Height*/, unsigned int* /*Format*/,
                                        unsigned int /*Levels*/, unsigned int /*Flags*/,
                                        void** OutHandle) {
  if (OutHandle) *OutHandle = nullptr;
  return GR_GENERIC;
}
GENRESULT RenderPipeline::DestroyTexture(void* /*Handle*/) { return GR_GENERIC; }
GENRESULT RenderPipeline::IsTexture(void* /*Handle*/) { return GR_GENERIC; }
GENRESULT RenderPipeline::LockTexture(void* /*Handle*/, int /*Level*/, void* /*Out*/) { return GR_GENERIC; }
GENRESULT RenderPipeline::UnlockTexture(void* /*Handle*/, int /*Level*/) { return GR_GENERIC; }
GENRESULT RenderPipeline::GetTextureFormat(void* /*Handle*/, int* /*OutFormat*/) { return GR_GENERIC; }
GENRESULT RenderPipeline::GetTextureDim(void* /*Handle*/, unsigned int* /*OutW*/, unsigned int* /*OutH*/,
                                        int* /*OutLevels*/) { return GR_GENERIC; }
GENRESULT RenderPipeline::GetTextureInterface(void* /*Handle*/, void* /*UnusedA*/, void* /*UnusedB*/) { return GR_GENERIC; }
GENRESULT RenderPipeline::SetTextureLevelData(void* /*Handle*/, int /*Level*/, int /*SrcW*/, int /*SrcH*/,
                                              unsigned int /*Pitch*/, unsigned int* /*SrcFormat*/,
                                              const void* /*Pixels*/, void* /*A*/, void* /*B*/) { return GR_GENERIC; }
GENRESULT RenderPipeline::BlitTexture(void* /*DstHandle*/, int /*DstLevel*/, void* /*SrcHandle*/, int /*SrcLevel*/) { return GR_GENERIC; }
GENRESULT RenderPipeline::SetRenderTarget(void* /*Handle*/, void* /*UnusedA*/, void* /*UnusedB*/) { return GR_OK; }
GENRESULT RenderPipeline::GetRenderTarget(void* /*Handle*/) { return GR_GENERIC; }

GENRESULT RenderPipeline::BeginScene() { return GR_OK; }
GENRESULT RenderPipeline::EndScene() { return GR_OK; }

GENRESULT RenderPipeline::ResetRenderStatesToDefaults() { return GR_OK; }
GENRESULT RenderPipeline::SetRenderState(D3DRENDERSTATETYPE State, int Value) {
  // Store by state index (best-effort; full state mapping is M5+).
  if (State >= 0 && State < 256) {
    render_state_cache_[State] = Value;
  }
  return GR_OK;
}
GENRESULT RenderPipeline::GetRenderState(D3DRENDERSTATETYPE State, int* Out) {
  if (!Out) return GR_INVALID_PARMS;
  if (static_cast<int>(State) < 0 || static_cast<int>(State) >= 256) return GR_INVALID_PARMS;
  *Out = render_state_cache_[State];
  return GR_OK;
}
GENRESULT RenderPipeline::SetTextureStageState(int /*Stage*/, int /*Type*/, int /*Value*/) { return GR_OK; }
GENRESULT RenderPipeline::GetTextureStageState(int /*Stage*/, int /*Type*/, int* Out) {
  if (Out) *Out = 0; return GR_OK;
}
GENRESULT RenderPipeline::SetTextureStageTransform(int /*Stage*/, const float* /*Matrix16*/) { return GR_OK; }
GENRESULT RenderPipeline::GetTextureStageTransform(int /*Stage*/, float* /*OutMatrix16*/) { return GR_GENERIC; }
GENRESULT RenderPipeline::SetTextureStageTexture(int /*Stage*/, void* /*Handle*/) { return GR_OK; }
GENRESULT RenderPipeline::GetTextureStageTexture(int /*Stage*/, void** OutHandle) {
  if (OutHandle) *OutHandle = nullptr; return GR_OK;
}
GENRESULT RenderPipeline::VerifyState() { return GR_OK; }

GENRESULT RenderPipeline::DrawPrimitive(unsigned long /*Prim*/, unsigned int /*Fvf*/, const void* /*Verts*/,
                                        unsigned int /*Count*/, void* /*Unused*/) {
  // Draw submission is performed in render_frame (clear+triangle). Returning OK keeps
  // engine call paths unblocked; full VB/UP draw routing is M5+ follow-on.
  return GR_OK;
}
GENRESULT RenderPipeline::DrawIndexedPrimitiveUp(unsigned long /*Prim*/, unsigned int /*Fvf*/,
                                                const void* /*Verts*/, unsigned int /*VertCount*/,
                                                const void* /*Indices*/, unsigned int /*IndexCount*/,
                                                unsigned long /*Unused*/) { return GR_OK; }
GENRESULT RenderPipeline::DrawPrimitiveVb(unsigned long /*Prim*/, void* /*Vb*/, unsigned int /*StartVertex*/,
                                          unsigned int /*Count*/, void* /*Unused*/) { return GR_OK; }
GENRESULT RenderPipeline::DrawIndexedPrimitiveVb(unsigned long /*Prim*/, void* /*Vb*/, unsigned int /*BaseVertex*/,
                                                 unsigned int /*NumVerts*/, const void* /*Indices*/,
                                                 unsigned int /*IndexCount*/, unsigned long /*Unused*/) { return GR_OK; }

GENRESULT RenderPipeline::AddLight(void* /*Handle*/) { return GR_GENERIC; }
GENRESULT RenderPipeline::RemoveLight(void* /*Handle*/) { return GR_GENERIC; }
GENRESULT RenderPipeline::UpdateLight(void* /*Handle*/) { return GR_GENERIC; }
GENRESULT RenderPipeline::SetWorldN(int /*WorldIndex*/, const float* M12) {
  if (M12) std::memcpy(world_, M12, sizeof(world_)); return GR_OK;
}

// IComponentFactory
GENRESULT RenderPipeline::CreateInstance(AGGDESC* Desc, void** Instance) {
  // Accept "DirectX8"/"IRenderPipeline" style creation for bring-up.
  if (!Instance) return GR_INVALID_PARMS;
  *Instance = nullptr;
  if (Desc) {
    // Best-effort: accept any descriptor; return this instance.
  }
  AddRef();
  *Instance = static_cast<RenderPipeline*>(this);
  return GR_OK;
}
GENRESULT RenderPipeline::QueryInterface(const char* iid, void** Out) {
  if (!Out) return GR_INVALID_PARMS;
  if (!iid || std::strcmp(iid, "RenderPipeline") == 0 ||
      std::strcmp(iid, "DirectX8") == 0) {
    *Out = static_cast<RenderPipeline*>(this);
    AddRef();
    return GR_OK;
  }
  if (std::strcmp(iid, "IComponentFactory") == 0) {
    *Out = static_cast<IComponentFactory*>(this);
    AddRef();
    return GR_OK;
  }
  *Out = nullptr;
  return GR_INTERFACE_UNSUPPORTED;
}

// Bring-up helpers
bool RenderPipeline::init_for_window(Window* window, const RenderPipelineConfig& config) {
  window_ = window;
  config_ = config;
  if (!window_) {
    set_error("init_for_window: null window");
    return false;
  }
  // Delegate to CreateBuffers using the window adapter handle.
  HWND hwnd = window->adapter_handle();
  int mode = 0, outmode = 0;
  GENRESULT gr = CreateBuffers(hwnd, &mode, &outmode);
  if (gr != GR_OK) {
    set_error("init_for_window: CreateBuffers failed gr=" + std::to_string((int)gr));
    return false;
  }
  tracked_w_ = config.initial_width;
  tracked_h_ = config.initial_height;
  if (tracked_w_ <= 0 || tracked_h_ <= 0) {
    window->get_size(tracked_w_, tracked_h_);
  }
  // Resize swapchain to requested dimensions if needed.
  if (tracked_w_ > 0 && tracked_h_ > 0 &&
      (static_cast<uint32_t>(tracked_w_) != vk_->swapchain_extent.width ||
       static_cast<uint32_t>(tracked_h_) != vk_->swapchain_extent.height)) {
    resize(tracked_w_, tracked_h_);
  }
  initialized_ = true;
  return true;
}

void RenderPipeline::shutdown() {
  if (!vk_) return;
  if (vk_->device != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(vk_->device);
    for (auto fb : vk_->framebuffers) if (fb) vkDestroyFramebuffer(vk_->device, fb, nullptr);
    vk_->framebuffers.clear();
    if (vk_->render_pass) vkDestroyRenderPass(vk_->device, vk_->render_pass, nullptr);
    vk_->render_pass = VK_NULL_HANDLE;
    if (vk_->command_pool) vkDestroyCommandPool(vk_->device, vk_->command_pool, nullptr);
    vk_->command_pool = VK_NULL_HANDLE;
    vk_->command_buffers.clear();
    for (auto iv : vk_->swapchain_image_views) if (iv) vkDestroyImageView(vk_->device, iv, nullptr);
    vk_->swapchain_image_views.clear();
    if (vk_->swapchain) vkDestroySwapchainKHR(vk_->device, vk_->swapchain, nullptr);
    vk_->swapchain = VK_NULL_HANDLE;
    if (vk_->image_available) vkDestroySemaphore(vk_->device, vk_->image_available, nullptr);
    vk_->image_available = VK_NULL_HANDLE;
    for (auto s : vk_->render_finished) if (s) vkDestroySemaphore(vk_->device, s, nullptr);
    vk_->render_finished.clear();
    if (vk_->in_flight) vkDestroyFence(vk_->device, vk_->in_flight, nullptr);
    vk_->in_flight = VK_NULL_HANDLE;
    vkDestroyDevice(vk_->device, nullptr);
    vk_->device = VK_NULL_HANDLE;
  }
  if (vk_->surface && vk_->instance) SDL_Vulkan_DestroySurface(vk_->instance, vk_->surface, nullptr);
  vk_->surface = VK_NULL_HANDLE;
  if (vk_->instance) vkDestroyInstance(vk_->instance, nullptr);
  vk_->instance = VK_NULL_HANDLE;
  delete vk_;
  vk_ = nullptr;
  initialized_ = false;
}

bool RenderPipeline::resize(int w, int h) {
  if (!vk_ || !vk_->device) return false;
  if (w <= 0 || h <= 0) return false;
  vkDeviceWaitIdle(vk_->device);
  // Recreate swapchain.
  // Destroy old framebuffers/image views.
  for (auto fb : vk_->framebuffers) if (fb) vkDestroyFramebuffer(vk_->device, fb, nullptr);
  vk_->framebuffers.clear();
  for (auto iv : vk_->swapchain_image_views) if (iv) vkDestroyImageView(vk_->device, iv, nullptr);
  vk_->swapchain_image_views.clear();
  if (vk_->swapchain) vkDestroySwapchainKHR(vk_->device, vk_->swapchain, nullptr);
  vk_->swapchain = VK_NULL_HANDLE;

  VkSurfaceCapabilitiesKHR caps{};
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_->gpu, vk_->surface, &caps);
  if (caps.currentExtent.width != UINT32_MAX) {
    vk_->swapchain_extent = caps.currentExtent;
  } else {
    vk_->swapchain_extent = {static_cast<uint32_t>(w > 0 ? w : 1), static_cast<uint32_t>(h > 0 ? h : 1)};
    if (vk_->swapchain_extent.width < caps.minImageExtent.width) vk_->swapchain_extent.width = caps.minImageExtent.width;
    if (vk_->swapchain_extent.height < caps.minImageExtent.height) vk_->swapchain_extent.height = caps.minImageExtent.height;
    if (caps.maxImageExtent.width > 0 && vk_->swapchain_extent.width > caps.maxImageExtent.width) vk_->swapchain_extent.width = caps.maxImageExtent.width;
    if (caps.maxImageExtent.height > 0 && vk_->swapchain_extent.height > caps.maxImageExtent.height) vk_->swapchain_extent.height = caps.maxImageExtent.height;
  }

  uint32_t img_count = caps.minImageCount + 1;
  if (caps.maxImageCount > 0 && img_count > caps.maxImageCount) img_count = caps.maxImageCount;
  VkSwapchainCreateInfoKHR sci{};
  sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  sci.surface = vk_->surface;
  sci.minImageCount = img_count;
  sci.imageFormat = vk_->swapchain_format;
  sci.imageColorSpace = vk_->swapchain_colorspace;
  sci.imageExtent = vk_->swapchain_extent;
  sci.imageArrayLayers = 1;
  sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  if (vk_->graphics_family != vk_->present_family) {
    sci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    uint32_t qf[2] = {vk_->graphics_family, vk_->present_family};
    sci.queueFamilyIndexCount = 2;
    sci.pQueueFamilyIndices = qf;
  } else {
    sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sci.queueFamilyIndexCount = 0;
    sci.pQueueFamilyIndices = nullptr;
  }
  sci.preTransform = caps.currentTransform;
  sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  sci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  sci.clipped = VK_TRUE;
  VkResult vr = vkCreateSwapchainKHR(vk_->device, &sci, nullptr, &vk_->swapchain);
  if (vr != VK_SUCCESS) {
    set_error(std::string("resize: vkCreateSwapchainKHR failed ") + vk_result_str(vr));
    return false;
  }
  uint32_t sc = 0;
  vkGetSwapchainImagesKHR(vk_->device, vk_->swapchain, &sc, nullptr);
  vk_->swapchain_images.resize(sc);
  vkGetSwapchainImagesKHR(vk_->device, vk_->swapchain, &sc, vk_->swapchain_images.data());
  vk_->swapchain_image_views.resize(sc);
  for (uint32_t i = 0; i < sc; ++i) {
    VkImageViewCreateInfo ivci{};
    ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.image = vk_->swapchain_images[i];
    ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format = vk_->swapchain_format;
    ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ivci.subresourceRange.baseMipLevel = 0;
    ivci.subresourceRange.levelCount = 1;
    ivci.subresourceRange.baseArrayLayer = 0;
    ivci.subresourceRange.layerCount = 1;
    vkCreateImageView(vk_->device, &ivci, nullptr, &vk_->swapchain_image_views[i]);
  }
  vk_->framebuffers.resize(sc);
  for (uint32_t i = 0; i < sc; ++i) {
    VkFramebufferCreateInfo fci{};
    fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fci.renderPass = vk_->render_pass;
    fci.attachmentCount = 1;
    fci.pAttachments = &vk_->swapchain_image_views[i];
    fci.width = vk_->swapchain_extent.width;
    fci.height = vk_->swapchain_extent.height;
    fci.layers = 1;
    vkCreateFramebuffer(vk_->device, &fci, nullptr, &vk_->framebuffers[i]);
  }
  return true;
}

bool RenderPipeline::render_frame(float /*t_seconds*/) {
  if (!initialized_ || !vk_ || !vk_->device) return false;
  auto t0 = std::chrono::steady_clock::now();
  VkDevice dev = vk_->device;
  // Wait for fence
  vkWaitForFences(dev, 1, &vk_->in_flight, VK_TRUE, UINT64_MAX);
  vkResetFences(dev, 1, &vk_->in_flight);

  uint32_t image_index = 0;
  VkResult vr = vkAcquireNextImageKHR(dev, vk_->swapchain, UINT64_MAX, vk_->image_available, VK_NULL_HANDLE, &image_index);
  if (vr == VK_ERROR_OUT_OF_DATE_KHR) {
    int w = 0, h = 0;
    if (window_) window_->get_size(w, h);
    if (w > 0 && h > 0) resize(w, h);
    {
      auto t1 = std::chrono::steady_clock::now();
      last_frame_ms_ = std::chrono::duration<double, std::milli>(t1 - t0).count();
    }
    return true;
  }
  if (vr != VK_SUCCESS && vr != VK_SUBOPTIMAL_KHR) {
    set_error(std::string("render_frame: acquire failed ") + vk_result_str(vr));
    {
      auto t1 = std::chrono::steady_clock::now();
      last_frame_ms_ = std::chrono::duration<double, std::milli>(t1 - t0).count();
    }
    return false;
  }
  vk_->current_image = image_index;

  VkCommandBuffer cmd = vk_->command_buffers[image_index];
  vkResetCommandBuffer(cmd, 0);
  VkCommandBufferBeginInfo bi{};
  bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(cmd, &bi);

  VkClearValue clear{};
  clear.color = {config_.clear_r, config_.clear_g, config_.clear_b, config_.clear_a};
  VkRenderPassBeginInfo rpbi{};
  rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  rpbi.renderPass = vk_->render_pass;
  rpbi.framebuffer = vk_->framebuffers[image_index];
  rpbi.renderArea.offset = {0, 0};
  rpbi.renderArea.extent = vk_->swapchain_extent;
  rpbi.clearValueCount = 1;
  rpbi.pClearValues = &clear;
  vkCmdBeginRenderPass(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);

  // Viewport (full swapchain).
  VkViewport vp{};
  vp.x = 0.0f; vp.y = 0.0f;
  vp.width = static_cast<float>(vk_->swapchain_extent.width);
  vp.height = static_cast<float>(vk_->swapchain_extent.height);
  vp.minDepth = 0.0f; vp.maxDepth = 1.0f;
  vkCmdSetViewport(cmd, 0, 1, &vp);
  VkRect2D scissor{{0, 0}, vk_->swapchain_extent};
  vkCmdSetScissor(cmd, 0, 1, &scissor);

  vkCmdEndRenderPass(cmd);
  vkEndCommandBuffer(cmd);

  VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkSubmitInfo si{};
  si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  si.waitSemaphoreCount = 1;
  si.pWaitSemaphores = &vk_->image_available;
  si.pWaitDstStageMask = &wait_stage;
  si.commandBufferCount = 1;
  si.pCommandBuffers = &cmd;
  VkSemaphore render_finished_sem = vk_->render_finished[image_index];
  si.signalSemaphoreCount = 1;
  si.pSignalSemaphores = &render_finished_sem;
  vr = vkQueueSubmit(vk_->graphics_queue, 1, &si, vk_->in_flight);
  if (vr != VK_SUCCESS) {
    set_error(std::string("render_frame: vkQueueSubmit failed ") + vk_result_str(vr));
    {
      auto t1 = std::chrono::steady_clock::now();
      last_frame_ms_ = std::chrono::duration<double, std::milli>(t1 - t0).count();
    }
    return false;
  }

  VkPresentInfoKHR pi{};
  pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  pi.waitSemaphoreCount = 1;
  VkSemaphore render_finished_sem2 = vk_->render_finished[image_index];
  pi.pWaitSemaphores = &render_finished_sem2;
  pi.swapchainCount = 1;
  pi.pSwapchains = &vk_->swapchain;
  pi.pImageIndices = &image_index;
  vr = vkQueuePresentKHR(vk_->present_queue, &pi);
  if (vr == VK_ERROR_OUT_OF_DATE_KHR || vr == VK_SUBOPTIMAL_KHR) {
    int w = 0, h = 0;
    if (window_) window_->get_size(w, h);
    if (w > 0 && h > 0) resize(w, h);
    {
      auto t1 = std::chrono::steady_clock::now();
      last_frame_ms_ = std::chrono::duration<double, std::milli>(t1 - t0).count();
      if (frame_count_ == 0) avg_frame_ms_ = last_frame_ms_; else avg_frame_ms_ = avg_frame_ms_ * 0.9 + last_frame_ms_ * 0.1;
      ++frame_count_;
    }
    return true;
  }
  if (vr != VK_SUCCESS) {
    set_error(std::string("render_frame: present failed ") + vk_result_str(vr));
    {
      auto t1 = std::chrono::steady_clock::now();
      last_frame_ms_ = std::chrono::duration<double, std::milli>(t1 - t0).count();
    }
    return false;
  }
  {
    auto t1 = std::chrono::steady_clock::now();
    last_frame_ms_ = std::chrono::duration<double, std::milli>(t1 - t0).count();
    if (frame_count_ == 0) avg_frame_ms_ = last_frame_ms_; else avg_frame_ms_ = avg_frame_ms_ * 0.9 + last_frame_ms_ * 0.1;
    ++frame_count_;
  }
  return true;
}

void RenderPipeline::set_clear_color(float r, float g, float b, float a) {
  config_.clear_r = r;
  config_.clear_g = g;
  config_.clear_b = b;
  config_.clear_a = a;
}

int RenderPipeline::swapchain_image_count() const {
  return vk_ ? static_cast<int>(vk_->swapchain_images.size()) : 0;
}

}  // namespace fl_port
