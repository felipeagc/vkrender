#include "window.hpp"
#include "context.hpp"
#include "util.hpp"
#include <SDL2/SDL_vulkan.h>
#include <util/log.hpp>
#include <util/time.hpp>
#include <vector>

static inline uint32_t
get_swapchain_num_images(const VkSurfaceCapabilitiesKHR &surfaceCapabilities) {
  uint32_t imageCount = surfaceCapabilities.minImageCount + 1;

  if (surfaceCapabilities.maxImageCount > 0 &&
      imageCount > surfaceCapabilities.maxImageCount) {
    imageCount = surfaceCapabilities.maxImageCount;
  }

  return imageCount;
}

static inline VkSurfaceFormatKHR
get_swapchain_format(const std::vector<VkSurfaceFormatKHR> &formats) {
  if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
    return {VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR};
  }

  for (const auto &format : formats) {
    if (format.format == VK_FORMAT_R8G8B8A8_UNORM) {
      return format;
    }
  }

  return formats[0];
}

static inline VkExtent2D get_swapchain_extent(
    uint32_t width,
    uint32_t height,
    const VkSurfaceCapabilitiesKHR &surfaceCapabilities) {
  if (surfaceCapabilities.currentExtent.width == static_cast<uint32_t>(-1)) {
    VkExtent2D swapchainExtent = {width, height};
    if (swapchainExtent.width < surfaceCapabilities.minImageExtent.width) {
      swapchainExtent.width = surfaceCapabilities.minImageExtent.width;
    }

    if (swapchainExtent.height < surfaceCapabilities.minImageExtent.height) {
      swapchainExtent.height = surfaceCapabilities.minImageExtent.height;
    }

    if (swapchainExtent.width > surfaceCapabilities.maxImageExtent.width) {
      swapchainExtent.width = surfaceCapabilities.maxImageExtent.width;
    }

    if (swapchainExtent.height > surfaceCapabilities.maxImageExtent.height) {
      swapchainExtent.height = surfaceCapabilities.maxImageExtent.height;
    }

    return swapchainExtent;
  }

  return surfaceCapabilities.currentExtent;
}

static inline VkImageUsageFlags
get_swapchain_usage_flags(const VkSurfaceCapabilitiesKHR &surfaceCapabilities) {
  if (surfaceCapabilities.supportedUsageFlags &
      VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
    return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
           VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  }

  ut_log_fatal(
      "VK_IMAGE_USAGE_TRANSFER_DST image usage is not supported by the "
      "swapchain!\n"
      "Supported swapchain image usages include:\n"
      "{}\n{}\n{}\n{}\n{}\n{}\n{}\n{}\n",
      (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT
           ? "    VK_IMAGE_USAGE_TRANSFER_SRC\n"
           : ""),
      (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT
           ? "    VK_IMAGE_USAGE_TRANSFER_DST\n"
           : ""),
      (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_SAMPLED_BIT
           ? "    VK_IMAGE_USAGE_SAMPLED\n"
           : ""),
      (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT
           ? "    VK_IMAGE_USAGE_STORAGE\n"
           : ""),
      (surfaceCapabilities.supportedUsageFlags &
               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
           ? "    VK_IMAGE_USAGE_COLOR_ATTACHMENT\n"
           : ""),
      (surfaceCapabilities.supportedUsageFlags &
               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
           ? "    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT\n"
           : ""),
      (surfaceCapabilities.supportedUsageFlags &
               VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT
           ? "    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT\n"
           : ""),
      (surfaceCapabilities.supportedUsageFlags &
               VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
           ? "    VK_IMAGE_USAGE_INPUT_ATTACHMENT"
           : ""));

  return static_cast<VkImageUsageFlags>(-1);
}

static inline VkSurfaceTransformFlagBitsKHR
get_swapchain_transform(const VkSurfaceCapabilitiesKHR &surfaceCapabilities) {
  if (surfaceCapabilities.supportedTransforms &
      VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
    return VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  } else {
    return surfaceCapabilities.currentTransform;
  }
}

static inline VkPresentModeKHR
get_swapchain_present_mode(const std::vector<VkPresentModeKHR> &presentModes) {
  for (const auto &presentMode : presentModes) {
    if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
      ut_log_debug("Recreating swapchain using immediate present mode");
      return presentMode;
    }
  }

  for (const auto &presentMode : presentModes) {
    if (presentMode == VK_PRESENT_MODE_FIFO_KHR) {
      ut_log_debug("Recreating swapchain using FIFO present mode");
      return presentMode;
    }
  }

  for (const auto &presentMode : presentModes) {
    if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      ut_log_debug("Recreating swapchain using mailbox present mode");
      return presentMode;
    }
  }

  ut_log_fatal("FIFO present mode is not supported by the swapchain!");

  return static_cast<VkPresentModeKHR>(-1);
}

static inline bool create_vulkan_surface(re_window_t *window) {
  return SDL_Vulkan_CreateSurface(
      window->sdl_window, g_ctx.instance, &window->surface);
}

static inline void create_sync_objects(re_window_t *window) {
  for (auto &resources : window->frame_resources) {
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;

    VK_CHECK(vkCreateSemaphore(
        g_ctx.device,
        &semaphoreCreateInfo,
        nullptr,
        &resources.image_available_semaphore));

    VK_CHECK(vkCreateSemaphore(
        g_ctx.device,
        &semaphoreCreateInfo,
        nullptr,
        &resources.rendering_finished_semaphore));

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.pNext = nullptr;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VK_CHECK(vkCreateFence(
        g_ctx.device, &fenceCreateInfo, nullptr, &resources.fence));
  }
}

static inline void
create_swapchain(re_window_t *window, uint32_t width, uint32_t height) {
  for (uint32_t i = 0; i < window->swapchain_image_count; i++) {
    vkDestroyImageView(g_ctx.device, window->swapchain_image_views[i], nullptr);
  }

  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      g_ctx.physical_device, window->surface, &surfaceCapabilities);

  uint32_t count;

  vkGetPhysicalDeviceSurfaceFormatsKHR(
      g_ctx.physical_device, window->surface, &count, nullptr);
  std::vector<VkSurfaceFormatKHR> surfaceFormats(count);
  vkGetPhysicalDeviceSurfaceFormatsKHR(
      g_ctx.physical_device, window->surface, &count, surfaceFormats.data());

  vkGetPhysicalDeviceSurfacePresentModesKHR(
      g_ctx.physical_device, window->surface, &count, nullptr);
  std::vector<VkPresentModeKHR> presentModes(count);
  vkGetPhysicalDeviceSurfacePresentModesKHR(
      g_ctx.physical_device, window->surface, &count, presentModes.data());

  auto desiredNumImages = get_swapchain_num_images(surfaceCapabilities);
  auto desiredFormat = get_swapchain_format(surfaceFormats);
  auto desiredExtent = get_swapchain_extent(width, height, surfaceCapabilities);
  auto desiredUsage = get_swapchain_usage_flags(surfaceCapabilities);
  auto desiredTransform = get_swapchain_transform(surfaceCapabilities);
  auto desiredPresentMode = get_swapchain_present_mode(presentModes);

  VkSwapchainKHR oldSwapchain = window->swapchain;

  VkSwapchainCreateInfoKHR createInfo{
      VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, // sType
      nullptr,                                     // pNext
      0,                                           // flags
      window->surface,
      desiredNumImages,                  // minImageCount
      desiredFormat.format,              // imageFormat
      desiredFormat.colorSpace,          // imageColorSpace
      desiredExtent,                     // imageExtent
      1,                                 // imageArrayLayers
      desiredUsage,                      // imageUsage
      VK_SHARING_MODE_EXCLUSIVE,         // imageSharingMode
      0,                                 // queueFamilyIndexCount
      nullptr,                           // pQueueFamiylIndices
      desiredTransform,                  // preTransform
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, // compositeAlpha
      desiredPresentMode,                // presentMode
      VK_TRUE,                           // clipped
      oldSwapchain                       // oldSwapchain
  };

  vkCreateSwapchainKHR(g_ctx.device, &createInfo, nullptr, &window->swapchain);

  if (oldSwapchain) {
    vkDestroySwapchainKHR(g_ctx.device, oldSwapchain, nullptr);
  }

  window->swapchain_image_format = desiredFormat.format;
  window->swapchain_extent = desiredExtent;

  VK_CHECK(vkGetSwapchainImagesKHR(
      g_ctx.device,
      window->swapchain,
      &window->swapchain_image_count,
      nullptr));

  window->swapchain_images =
      (VkImage *)malloc(sizeof(VkImage) * window->swapchain_image_count);
  VK_CHECK(vkGetSwapchainImagesKHR(
      g_ctx.device, window->swapchain, &count, window->swapchain_images));
}

static inline void create_swapchain_image_views(re_window_t *window) {
  window->swapchain_image_views = (VkImageView *)malloc(
      sizeof(VkImageView) * window->swapchain_image_count);

  for (size_t i = 0; i < window->swapchain_image_count; i++) {
    VkImageViewCreateInfo createInfo{
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, // sType
        nullptr,                                  // pNext
        0,                                        // flags
        window->swapchain_images[i],
        VK_IMAGE_VIEW_TYPE_2D,
        window->swapchain_image_format,
        {
            VK_COMPONENT_SWIZZLE_IDENTITY, // r
            VK_COMPONENT_SWIZZLE_IDENTITY, // g
            VK_COMPONENT_SWIZZLE_IDENTITY, // b
            VK_COMPONENT_SWIZZLE_IDENTITY, // a
        },
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    VK_CHECK(vkCreateImageView(
        g_ctx.device, &createInfo, nullptr, &window->swapchain_image_views[i]));
  }
}

static inline void allocate_graphics_command_buffers(re_window_t *window) {
  VkCommandBufferAllocateInfo allocateInfo = {};
  allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocateInfo.pNext = nullptr;
  allocateInfo.commandPool = g_ctx.graphics_command_pool;
  allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocateInfo.commandBufferCount = RE_MAX_FRAMES_IN_FLIGHT;

  std::vector<VkCommandBuffer> commandBuffers(RE_MAX_FRAMES_IN_FLIGHT);

  vkAllocateCommandBuffers(g_ctx.device, &allocateInfo, commandBuffers.data());

  for (size_t i = 0; i < RE_MAX_FRAMES_IN_FLIGHT; i++) {
    window->frame_resources[i].command_buffer = commandBuffers[i];
  }
}

// Populates the depthStencil member struct
static inline void create_depth_stencil_resources(re_window_t *window) {
  assert(re_context_get_supported_depth_format(&g_ctx, &window->depth_format));

  VkImageCreateInfo imageCreateInfo = {
      VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, // sType
      nullptr,                             // pNext
      0,                                   // flags
      VK_IMAGE_TYPE_2D,
      window->depth_format,
      {
          window->swapchain_extent.width,
          window->swapchain_extent.height,
          1,
      },
      1,
      1,
      VK_SAMPLE_COUNT_1_BIT,
      VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      VK_SHARING_MODE_EXCLUSIVE,
      0,
      nullptr,
      VK_IMAGE_LAYOUT_UNDEFINED,
  };

  VmaAllocationCreateInfo allocInfo{};
  allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  VK_CHECK(vmaCreateImage(
      g_ctx.gpu_allocator,
      &imageCreateInfo,
      &allocInfo,
      &window->depth_stencil.image,
      &window->depth_stencil.allocation,
      nullptr));

  VkImageViewCreateInfo imageViewCreateInfo = {
      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, // sType
      nullptr,                                  // pNext
      0,                                        // flags
      window->depth_stencil.image,
      VK_IMAGE_VIEW_TYPE_2D,
      window->depth_format,
      {
          VK_COMPONENT_SWIZZLE_IDENTITY, // r
          VK_COMPONENT_SWIZZLE_IDENTITY, // g
          VK_COMPONENT_SWIZZLE_IDENTITY, // b
          VK_COMPONENT_SWIZZLE_IDENTITY, // a
      },                                 // components
      {
          VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, // aspectMask
          0, // baseMipLevel
          1, // levelCount
          0, // baseArrayLayer
          1, // layerCount
      },     // subresourceRange
  };

  VK_CHECK(vkCreateImageView(
      g_ctx.device,
      &imageViewCreateInfo,
      nullptr,
      &window->depth_stencil.view));
}

static inline void create_render_pass(re_window_t *window) {
  window->render_target.sample_count = VK_SAMPLE_COUNT_1_BIT;

  VkAttachmentDescription attachmentDescriptions[] = {
      // Resolved color attachment
      VkAttachmentDescription{
          0,                                // flags
          window->swapchain_image_format,   // format
          VK_SAMPLE_COUNT_1_BIT,            // samples
          VK_ATTACHMENT_LOAD_OP_CLEAR,      // loadOp
          VK_ATTACHMENT_STORE_OP_STORE,     // storeOp
          VK_ATTACHMENT_LOAD_OP_DONT_CARE,  // stencilLoadOp
          VK_ATTACHMENT_STORE_OP_DONT_CARE, // stencilStoreOp
          VK_IMAGE_LAYOUT_UNDEFINED,        // initialLayout
          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,  // finalLayout
      },

      // Resolved depth attachment
      VkAttachmentDescription{
          0,                                                // flags
          window->depth_format,                             // format
          VK_SAMPLE_COUNT_1_BIT,                            // samples
          VK_ATTACHMENT_LOAD_OP_CLEAR,                      // loadOp
          VK_ATTACHMENT_STORE_OP_STORE,                     // storeOp
          VK_ATTACHMENT_LOAD_OP_DONT_CARE,                  // stencilLoadOp
          VK_ATTACHMENT_STORE_OP_DONT_CARE,                 // stencilStoreOp
          VK_IMAGE_LAYOUT_UNDEFINED,                        // initialLayout
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, // finalLayout
      },
  };

  VkAttachmentReference colorAttachmentReference = {
      0,                                        // attachment
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // layout
  };

  VkAttachmentReference depthAttachmentReference = {
      1,                                                // attachment
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, // layout
  };

  VkSubpassDescription subpassDescription = {
      {},                              // flags
      VK_PIPELINE_BIND_POINT_GRAPHICS, // pipelineBindPoint
      0,                               // inputAttachmentCount
      nullptr,                         // pInputAttachments
      1,                               // colorAttachmentCount
      &colorAttachmentReference,       // pColorAttachments
      nullptr,                         // pResolveAttachments
      &depthAttachmentReference,       // pDepthStencilAttachment
      0,                               // preserveAttachmentCount
      nullptr,                         // pPreserveAttachments
  };

  VkSubpassDependency dependencies[] = {
      VkSubpassDependency{
          VK_SUBPASS_EXTERNAL,                           // srcSubpass
          0,                                             // dstSubpass
          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,          // srcStageMask
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // dstStageMask
          VK_ACCESS_MEMORY_READ_BIT,                     // srcAccessMask
          VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
              VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, // dstAccessMask
          VK_DEPENDENCY_BY_REGION_BIT,              // dependencyFlags
      },
      VkSubpassDependency{
          0,                                             // srcSubpass
          VK_SUBPASS_EXTERNAL,                           // dstSubpass
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // srcStageMask
          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,          // dstStageMask
          VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
              VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, // srcAccessMask
          VK_ACCESS_MEMORY_READ_BIT,                // dstAccessMask
          VK_DEPENDENCY_BY_REGION_BIT,              // dependencyFlags
      },
  };

  VkRenderPassCreateInfo renderPassCreateInfo = {
      VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, // sType
      nullptr,                                   // pNext
      0,                                         // flags
      static_cast<uint32_t>(
          ARRAYSIZE(attachmentDescriptions)),         // attachmentCount
      attachmentDescriptions,                         // pAttachments
      1,                                              // subpassCount
      &subpassDescription,                            // pSubpasses
      static_cast<uint32_t>(ARRAYSIZE(dependencies)), // dependencyCount
      dependencies,                                   // pDependencies
  };

  VK_CHECK(vkCreateRenderPass(
      g_ctx.device,
      &renderPassCreateInfo,
      nullptr,
      &window->render_target.render_pass));
}

static inline void regen_framebuffer(
    re_window_t *window,
    VkFramebuffer &framebuffer,
    VkImageView &swapchainImageView) {
  vkDestroyFramebuffer(g_ctx.device, framebuffer, nullptr);

  VkImageView attachments[]{
      swapchainImageView,
      window->depth_stencil.view,
  };

  VkFramebufferCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,     // sType
      nullptr,                                       // pNext
      0,                                             // flags
      window->render_target.render_pass,             // renderPass
      static_cast<uint32_t>(ARRAYSIZE(attachments)), // attachmentCount
      attachments,                                   // pAttachments
      window->swapchain_extent.width,                // width
      window->swapchain_extent.height,               // height
      1,                                             // layers
  };

  VK_CHECK(
      vkCreateFramebuffer(g_ctx.device, &createInfo, nullptr, &framebuffer));
}

static inline void destroy_resizables(re_window_t *window) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  for (auto &resources : window->frame_resources) {
    vkFreeCommandBuffers(
        g_ctx.device,
        g_ctx.graphics_command_pool,
        1,
        &resources.command_buffer);
  }

  if (window->depth_stencil.image) {
    vkDestroyImageView(g_ctx.device, window->depth_stencil.view, nullptr);
    vmaDestroyImage(
        g_ctx.gpu_allocator,
        window->depth_stencil.image,
        window->depth_stencil.allocation);
    window->depth_stencil.image = nullptr;
    window->depth_stencil.allocation = VK_NULL_HANDLE;
  }

  vkDestroyRenderPass(g_ctx.device, window->render_target.render_pass, nullptr);
}

// When window gets resized, call this.
static inline void update_size(re_window_t *window) {
  destroy_resizables(window);

  uint32_t width, height;
  re_window_get_size(window, &width, &height);
  create_swapchain(window, width, height);
  create_swapchain_image_views(window);
  create_depth_stencil_resources(window);
  create_render_pass(window);
  allocate_graphics_command_buffers(window);
}

bool re_window_init(
    re_window_t *window, const char *title, uint32_t width, uint32_t height) {
  window->should_close = false;
  window->clear_color = glm::vec4(0.0, 0.0, 0.0, 1.0);
  window->delta_time = 0.0f;
  window->time_before_ns = 0;

  window->swapchain = VK_NULL_HANDLE;
  window->surface = VK_NULL_HANDLE;
  window->swapchain_image_count = 0;

  window->current_frame = 0;
  // Index of the current swapchain image
  window->current_image_index = 0;

  for (uint32_t i = 0; i < RE_MAX_FRAMES_IN_FLIGHT; i++) {
    window->frame_resources[i].framebuffer = VK_NULL_HANDLE;
    window->frame_resources[i].command_buffer = VK_NULL_HANDLE;
  }

  auto subsystems = SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER;
  if (subsystems == SDL_WasInit(0)) {
    SDL_Init(subsystems);
  }

  window->sdl_window = SDL_CreateWindow(
      title,
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      width,
      height,
      SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);

  if (window->sdl_window == NULL) {
    return false;
  }

  SDL_SetWindowResizable(window->sdl_window, SDL_TRUE);

  uint32_t sdl_extension_count = 0;
  SDL_Vulkan_GetInstanceExtensions(
      window->sdl_window, &sdl_extension_count, nullptr);
  const char **sdl_extensions =
      (const char **)malloc(sizeof(const char *) * sdl_extension_count);
  SDL_Vulkan_GetInstanceExtensions(
      window->sdl_window, &sdl_extension_count, sdl_extensions);

  // These context initialization functions only run if the context is
  // uninitialized
  re_context_pre_init(&g_ctx, sdl_extensions, sdl_extension_count);

  free(sdl_extensions);

  create_vulkan_surface(window);

  // Lazily create vulkan context stuff
  re_context_late_inint(&g_ctx, window->surface);

  VkBool32 supported;
  vkGetPhysicalDeviceSurfaceSupportKHR(
      g_ctx.physical_device,
      g_ctx.present_queue_family_index,
      window->surface,
      &supported);
  if (!supported) {
    return false;
  }

  window->max_samples = re_context_get_max_sample_count(&g_ctx);

  create_sync_objects(window);

  create_swapchain(window, width, height);
  create_swapchain_image_views(window);

  allocate_graphics_command_buffers(window);

  create_depth_stencil_resources(window);

  create_render_pass(window);

  return true;
}

void re_window_get_size(
    const re_window_t *window, uint32_t *width, uint32_t *height) {
  int iwidth, iheight;
  SDL_GetWindowSize(window->sdl_window, &iwidth, &iheight);
  *width = (uint32_t)iwidth;
  *height = (uint32_t)iheight;
}

void re_window_destroy(re_window_t *window) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  destroy_resizables(window);

  for (uint32_t i = 0; i < window->swapchain_image_count; i++) {
    vkDestroyImageView(g_ctx.device, window->swapchain_image_views[i], nullptr);
  }

  vkDestroySwapchainKHR(g_ctx.device, window->swapchain, nullptr);

  for (auto &frameResource : window->frame_resources) {
    vkDestroyFramebuffer(g_ctx.device, frameResource.framebuffer, nullptr);
    vkDestroySemaphore(
        g_ctx.device, frameResource.image_available_semaphore, nullptr);
    vkDestroySemaphore(
        g_ctx.device, frameResource.rendering_finished_semaphore, nullptr);
    vkDestroyFence(g_ctx.device, frameResource.fence, nullptr);
  }

  vkDestroySurfaceKHR(g_ctx.instance, window->surface, nullptr);

  SDL_DestroyWindow(window->sdl_window);

  free(window->swapchain_images);
  free(window->swapchain_image_views);
}

bool re_window_poll_event(re_window_t *window, SDL_Event *event) {
  bool result = SDL_PollEvent(event);

  switch (event->type) {
  case SDL_WINDOWEVENT:
    switch (event->window.event) {
    case SDL_WINDOWEVENT_RESIZED:
      update_size(window);
      break;
    }
    break;
  }

  return result;
}

void re_window_begin_frame(re_window_t *window) {
  window->time_before_ns = ut_time_ns();

  // Begin
  vkWaitForFences(
      g_ctx.device,
      1,
      &window->frame_resources[window->current_frame].fence,
      VK_TRUE,
      UINT64_MAX);

  vkResetFences(
      g_ctx.device, 1, &window->frame_resources[window->current_frame].fence);

  if (vkAcquireNextImageKHR(
          g_ctx.device,
          window->swapchain,
          UINT64_MAX,
          window->frame_resources[window->current_frame]
              .image_available_semaphore,
          VK_NULL_HANDLE,
          &window->current_image_index) == VK_ERROR_OUT_OF_DATE_KHR) {
    update_size(window);
  }

  VkImageSubresourceRange imageSubresourceRange{
      VK_IMAGE_ASPECT_COLOR_BIT, // aspectMask
      0,                         // baseMipLevel
      1,                         // levelCount
      0,                         // baseArrayLayer
      1,                         // layerCount
  };

  regen_framebuffer(
      window,
      window->frame_resources[window->current_frame].framebuffer,
      window->swapchain_image_views[window->current_image_index]);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  beginInfo.pInheritanceInfo = nullptr;

  auto &commandBuffer =
      window->frame_resources[window->current_frame].command_buffer;

  VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

  if (g_ctx.present_queue != g_ctx.graphics_queue) {
    VkImageMemoryBarrier barrierFromPresentToDraw = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,   // sType
        nullptr,                                  // pNext
        VK_ACCESS_MEMORY_READ_BIT,                // srcAccessMask
        VK_ACCESS_MEMORY_READ_BIT,                // dstAccessMask
        VK_IMAGE_LAYOUT_UNDEFINED,                // oldLayout
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // newLayout
        g_ctx.present_queue_family_index,         // srcQueueFamilyIndex
        g_ctx.graphics_queue_family_index,        // dstQueueFamilyIndex
        window->swapchain_images[window->current_image_index], // image
        imageSubresourceRange, // subresourceRange
    };

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrierFromPresentToDraw);
  }
}

void re_window_end_frame(re_window_t *window) {
  auto &commandBuffer =
      window->frame_resources[window->current_frame].command_buffer;

  VkImageSubresourceRange imageSubresourceRange{
      VK_IMAGE_ASPECT_COLOR_BIT, // aspectMask
      0,                         // baseMipLevel
      1,                         // levelCount
      0,                         // baseArrayLayer
      1,                         // layerCount
  };

  if (g_ctx.present_queue != g_ctx.graphics_queue) {
    VkImageMemoryBarrier barrierFromDrawToPresent{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,   // sType
        nullptr,                                  // pNext
        VK_ACCESS_MEMORY_READ_BIT,                // srcAccessMask
        VK_ACCESS_MEMORY_READ_BIT,                // dstAccessMask
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // oldLayout
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,          // newLayout
        g_ctx.graphics_queue_family_index,        // srcQueueFamilyIndex
        g_ctx.present_queue_family_index,         // dstQueueFamilyIndex
        window->swapchain_images[window->current_image_index], // image
        imageSubresourceRange, // subresourceRange
    };

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrierFromDrawToPresent);
  }

  VK_CHECK(vkEndCommandBuffer(commandBuffer));

  // Present
  VkPipelineStageFlags waitDstStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  VkSubmitInfo submitInfo = {
      VK_STRUCTURE_TYPE_SUBMIT_INFO, // sType
      nullptr,                       // pNext
      1,                             // waitSemaphoreCount
      &window->frame_resources[window->current_frame]
           .image_available_semaphore, // pWaitSemaphores
      &waitDstStageMask,               // pWaitDstStageMask
      1,                               // commandBufferCount
      &commandBuffer,                  // pCommandBuffers
      1,                               // signalSemaphoreCount
      &window->frame_resources[window->current_frame]
           .rendering_finished_semaphore, // pSignalSemaphores
  };

  g_ctx.queue_mutex.lock();
  vkQueueSubmit(
      g_ctx.graphics_queue,
      1,
      &submitInfo,
      window->frame_resources[window->current_frame].fence);

  VkPresentInfoKHR presentInfo = {
      VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      nullptr, // pNext
      1,       // waitSemaphoreCount
      &window->frame_resources[window->current_frame]
           .rendering_finished_semaphore, // pWaitSemaphores
      1,                                  // swapchainCount
      &window->swapchain,                 // pSwapchains
      &window->current_image_index,       // pImageIndices
      nullptr,                            // pResults
  };

  VkResult result = vkQueuePresentKHR(g_ctx.present_queue, &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    update_size(window);
  } else {
    assert(result == VK_SUCCESS);
  }
  g_ctx.queue_mutex.unlock();

  window->current_frame = (window->current_frame + 1) % RE_MAX_FRAMES_IN_FLIGHT;

  uint64_t elapsed_time_ns = ut_time_ns() - window->time_before_ns;

  window->delta_time = (double)elapsed_time_ns / 1.0e9;
}

void re_window_begin_render_pass(re_window_t *window) {
  auto &commandBuffer =
      window->frame_resources[window->current_frame].command_buffer;

  VkClearValue clearValues[2] = {};
  clearValues[0].color = {{
      window->clear_color.x,
      window->clear_color.y,
      window->clear_color.z,
      window->clear_color.w,
  }};
  clearValues[1].depthStencil = {1.0f, 0};

  VkRenderPassBeginInfo renderPassBeginInfo = {
      VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,                   // sType
      nullptr,                                                    // pNext
      window->render_target.render_pass,                          // renderPass
      window->frame_resources[window->current_frame].framebuffer, // framebuffer
      {{0, 0}, window->swapchain_extent},                         // renderArea
      ARRAYSIZE(clearValues), // clearValueCount
      clearValues,            // pClearValues
  };

  vkCmdBeginRenderPass(
      commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport{
      0.0f,                                                // x
      0.0f,                                                // y
      static_cast<float>(window->swapchain_extent.width),  // width
      static_cast<float>(window->swapchain_extent.height), // height
      0.0f,                                                // minDepth
      1.0f,                                                // maxDepth
  };

  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{{0, 0}, window->swapchain_extent};

  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void re_window_end_render_pass(re_window_t *window) {
  auto &command_buffer =
      window->frame_resources[window->current_frame].command_buffer;

  // End
  vkCmdEndRenderPass(command_buffer);
}

bool re_window_get_relative_mouse(const re_window_t *) {
  return SDL_GetRelativeMouseMode();
}

void re_window_set_relative_mouse(re_window_t *, bool relative) {
  SDL_SetRelativeMouseMode((SDL_bool)relative);
}

void re_window_get_mouse_state(const re_window_t *, int *x, int *y) {
  SDL_GetMouseState(x, y);
}

void re_window_get_relative_mouse_state(const re_window_t *, int *x, int *y) {
  SDL_GetRelativeMouseState(x, y);
}

void re_window_warp_mouse(re_window_t *window, int x, int y) {
  SDL_WarpMouseInWindow(window->sdl_window, x, y);
}

bool re_window_is_mouse_left_pressed(const re_window_t *) {
  auto state = SDL_GetMouseState(nullptr, nullptr);
  return (state & SDL_BUTTON(SDL_BUTTON_LEFT));
}

bool re_window_is_mouse_right_pressed(const re_window_t *) {
  auto state = SDL_GetMouseState(nullptr, nullptr);
  return (state & SDL_BUTTON(SDL_BUTTON_RIGHT));
}

bool re_window_is_scancode_pressed(const re_window_t *, SDL_Scancode scancode) {
  const Uint8 *state = SDL_GetKeyboardState(nullptr);
  return (bool)state[scancode];
}

VkCommandBuffer
re_window_get_current_command_buffer(const re_window_t *window) {
  return window->frame_resources[window->current_frame].command_buffer;
}
