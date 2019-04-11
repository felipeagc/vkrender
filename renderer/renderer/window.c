#include "window.h"
#include "context.h"
#include "util.h"
#include <fstd_util.h>
#include <gmath.h>

static inline uint32_t
get_swapchain_num_images(VkSurfaceCapabilitiesKHR *surface_capabilities) {
  uint32_t image_count = surface_capabilities->minImageCount + 1;

  if (surface_capabilities->maxImageCount > 0 &&
      image_count > surface_capabilities->maxImageCount) {
    image_count = surface_capabilities->maxImageCount;
  }

  return image_count;
}

static inline VkSurfaceFormatKHR
get_swapchain_format(VkSurfaceFormatKHR *formats, uint32_t format_count) {
  if (format_count == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
    return (VkSurfaceFormatKHR){VK_FORMAT_R8G8B8A8_UNORM,
                                VK_COLORSPACE_SRGB_NONLINEAR_KHR};
  }

  for (uint32_t i = 0; i < format_count; i++) {
    if (formats[i].format == VK_FORMAT_R8G8B8A8_UNORM) {
      return formats[i];
    }
  }

  return formats[0];
}

static inline VkExtent2D get_swapchain_extent(
    uint32_t width,
    uint32_t height,
    VkSurfaceCapabilitiesKHR *surface_capabilities) {
  if (surface_capabilities->currentExtent.width == (uint32_t)-1) {
    VkExtent2D swapchain_extent = {width, height};
    if (swapchain_extent.width < surface_capabilities->minImageExtent.width) {
      swapchain_extent.width = surface_capabilities->minImageExtent.width;
    }

    if (swapchain_extent.height < surface_capabilities->minImageExtent.height) {
      swapchain_extent.height = surface_capabilities->minImageExtent.height;
    }

    if (swapchain_extent.width > surface_capabilities->maxImageExtent.width) {
      swapchain_extent.width = surface_capabilities->maxImageExtent.width;
    }

    if (swapchain_extent.height > surface_capabilities->maxImageExtent.height) {
      swapchain_extent.height = surface_capabilities->maxImageExtent.height;
    }

    return swapchain_extent;
  }

  return surface_capabilities->currentExtent;
}

static inline VkImageUsageFlags
get_swapchain_usage_flags(VkSurfaceCapabilitiesKHR *surface_capabilities) {
  if (surface_capabilities->supportedUsageFlags &
      VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
    return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
           VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  }

  RE_LOG_FATAL(
      "VK_IMAGE_USAGE_TRANSFER_DST image usage is not supported by the "
      "swapchain!\n"
      "Supported swapchain image usages include:\n"
      "%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n",
      (surface_capabilities->supportedUsageFlags &
               VK_IMAGE_USAGE_TRANSFER_SRC_BIT
           ? "    VK_IMAGE_USAGE_TRANSFER_SRC\n"
           : ""),
      (surface_capabilities->supportedUsageFlags &
               VK_IMAGE_USAGE_TRANSFER_DST_BIT
           ? "    VK_IMAGE_USAGE_TRANSFER_DST\n"
           : ""),
      (surface_capabilities->supportedUsageFlags & VK_IMAGE_USAGE_SAMPLED_BIT
           ? "    VK_IMAGE_USAGE_SAMPLED\n"
           : ""),
      (surface_capabilities->supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT
           ? "    VK_IMAGE_USAGE_STORAGE\n"
           : ""),
      (surface_capabilities->supportedUsageFlags &
               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
           ? "    VK_IMAGE_USAGE_COLOR_ATTACHMENT\n"
           : ""),
      (surface_capabilities->supportedUsageFlags &
               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
           ? "    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT\n"
           : ""),
      (surface_capabilities->supportedUsageFlags &
               VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT
           ? "    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT\n"
           : ""),
      (surface_capabilities->supportedUsageFlags &
               VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
           ? "    VK_IMAGE_USAGE_INPUT_ATTACHMENT"
           : ""));

  return (VkImageUsageFlags)-1;
}

static inline VkSurfaceTransformFlagBitsKHR
get_swapchain_transform(VkSurfaceCapabilitiesKHR *surface_capabilities) {
  if (surface_capabilities->supportedTransforms &
      VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
    return VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  } else {
    return surface_capabilities->currentTransform;
  }
}

static inline VkPresentModeKHR get_swapchain_present_mode(
    VkPresentModeKHR *present_modes, uint32_t present_mode_count) {
  for (uint32_t i = 0; i < present_mode_count; i++) {
    if (present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
      RE_LOG_DEBUG("Recreating swapchain using immediate present mode");
      return present_modes[i];
    }
  }

  for (uint32_t i = 0; i < present_mode_count; i++) {
    if (present_modes[i] == VK_PRESENT_MODE_FIFO_KHR) {
      RE_LOG_DEBUG("Recreating swapchain using FIFO present mode");
      return present_modes[i];
    }
  }

  for (uint32_t i = 0; i < present_mode_count; i++) {
    if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      RE_LOG_DEBUG("Recreating swapchain using mailbox present mode");
      return present_modes[i];
    }
  }

  RE_LOG_FATAL("FIFO present mode is not supported by the swapchain!");

  return (VkPresentModeKHR)-1;
}

static inline VkResult create_vulkan_surface(re_window_t *window) {
  return glfwCreateWindowSurface(
      g_ctx.instance, window->glfw_window, NULL, &window->surface);
}

static inline void create_sync_objects(re_window_t *window) {
  for (uint32_t i = 0; i < ARRAY_SIZE(window->frame_resources); i++) {
    re_frame_resources_t *resources = &window->frame_resources[i];

    VkSemaphoreCreateInfo semaphore_create_info = {0};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = NULL;
    semaphore_create_info.flags = 0;

    VK_CHECK(vkCreateSemaphore(
        g_ctx.device,
        &semaphore_create_info,
        NULL,
        &resources->image_available_semaphore));

    VK_CHECK(vkCreateSemaphore(
        g_ctx.device,
        &semaphore_create_info,
        NULL,
        &resources->rendering_finished_semaphore));

    VkFenceCreateInfo fence_create_info = {0};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = NULL;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VK_CHECK(vkCreateFence(
        g_ctx.device, &fence_create_info, NULL, &resources->fence));
  }
}

static inline void
create_swapchain(re_window_t *window, uint32_t width, uint32_t height) {
  for (uint32_t i = 0; i < window->swapchain_image_count; i++) {
    vkDestroyImageView(g_ctx.device, window->swapchain_image_views[i], NULL);
  }

  VkSurfaceCapabilitiesKHR surface_capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      g_ctx.physical_device, window->surface, &surface_capabilities);

  uint32_t surface_format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(
      g_ctx.physical_device, window->surface, &surface_format_count, NULL);
  VkSurfaceFormatKHR *surface_formats = (VkSurfaceFormatKHR *)malloc(
      sizeof(VkSurfaceFormatKHR) * surface_format_count);
  vkGetPhysicalDeviceSurfaceFormatsKHR(
      g_ctx.physical_device,
      window->surface,
      &surface_format_count,
      surface_formats);

  uint32_t present_mode_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(
      g_ctx.physical_device, window->surface, &present_mode_count, NULL);
  VkPresentModeKHR *present_modes =
      (VkPresentModeKHR *)malloc(sizeof(VkPresentModeKHR) * present_mode_count);
  vkGetPhysicalDeviceSurfacePresentModesKHR(
      g_ctx.physical_device,
      window->surface,
      &present_mode_count,
      present_modes);

  uint32_t desired_num_images = get_swapchain_num_images(&surface_capabilities);
  VkSurfaceFormatKHR desired_format =
      get_swapchain_format(surface_formats, surface_format_count);
  VkExtent2D desired_extent =
      get_swapchain_extent(width, height, &surface_capabilities);
  VkImageUsageFlags desired_usage =
      get_swapchain_usage_flags(&surface_capabilities);
  VkSurfaceTransformFlagBitsKHR desired_transform =
      get_swapchain_transform(&surface_capabilities);
  VkPresentModeKHR desired_present_mode =
      get_swapchain_present_mode(present_modes, present_mode_count);

  VkSwapchainKHR old_swapchain = window->swapchain;

  VkSwapchainCreateInfoKHR create_info = {
      VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, // sType
      NULL,                                        // pNext
      0,                                           // flags
      window->surface,
      desired_num_images,                // minImageCount
      desired_format.format,             // imageFormat
      desired_format.colorSpace,         // imageColorSpace
      desired_extent,                    // imageExtent
      1,                                 // imageArrayLayers
      desired_usage,                     // imageUsage
      VK_SHARING_MODE_EXCLUSIVE,         // imageSharingMode
      0,                                 // queueFamilyIndexCount
      NULL,                              // pQueueFamiylIndices
      desired_transform,                 // preTransform
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, // compositeAlpha
      desired_present_mode,              // presentMode
      VK_TRUE,                           // clipped
      old_swapchain                      // oldSwapchain
  };

  vkCreateSwapchainKHR(g_ctx.device, &create_info, NULL, &window->swapchain);

  if (old_swapchain) {
    vkDestroySwapchainKHR(g_ctx.device, old_swapchain, NULL);
  }

  window->swapchain_image_format = desired_format.format;
  window->swapchain_extent = desired_extent;

  VK_CHECK(vkGetSwapchainImagesKHR(
      g_ctx.device, window->swapchain, &window->swapchain_image_count, NULL));

  if (window->swapchain_images != NULL) {
    free(window->swapchain_images);
  }
  window->swapchain_images =
      (VkImage *)malloc(sizeof(VkImage) * window->swapchain_image_count);
  VK_CHECK(vkGetSwapchainImagesKHR(
      g_ctx.device,
      window->swapchain,
      &window->swapchain_image_count,
      window->swapchain_images));

  free(present_modes);
  free(surface_formats);
}

static inline void create_swapchain_image_views(re_window_t *window) {
  if (window->swapchain_image_views != NULL) {
    free(window->swapchain_image_views);
  }
  window->swapchain_image_views = (VkImageView *)malloc(
      sizeof(VkImageView) * window->swapchain_image_count);

  for (uint32_t i = 0; i < window->swapchain_image_count; i++) {
    VkImageViewCreateInfo create_info = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, // sType
        NULL,                                     // pNext
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
        g_ctx.device, &create_info, NULL, &window->swapchain_image_views[i]));
  }
}

static inline void allocate_graphics_command_buffers(re_window_t *window) {
  VkCommandBufferAllocateInfo allocate_info = {0};
  allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocate_info.pNext = NULL;
  allocate_info.commandPool = g_ctx.graphics_command_pool;
  allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocate_info.commandBufferCount = RE_MAX_FRAMES_IN_FLIGHT;

  VkCommandBuffer command_buffers[RE_MAX_FRAMES_IN_FLIGHT];

  vkAllocateCommandBuffers(g_ctx.device, &allocate_info, command_buffers);

  for (uint32_t i = 0; i < ARRAY_SIZE(window->frame_resources); i++) {
    window->frame_resources[i].command_buffer = command_buffers[i];
  }
}

// Populates the depthStencil member struct
static inline void create_depth_stencil_resources(re_window_t *window) {
  assert(re_context_get_supported_depth_format(&window->depth_format));

  VkImageCreateInfo image_create_info = {
      VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, // sType
      NULL,                                // pNext
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
      NULL,
      VK_IMAGE_LAYOUT_UNDEFINED,
  };

  VmaAllocationCreateInfo alloc_info = {0};
  alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  VK_CHECK(vmaCreateImage(
      g_ctx.gpu_allocator,
      &image_create_info,
      &alloc_info,
      &window->depth_stencil.image,
      &window->depth_stencil.allocation,
      NULL));

  VkImageViewCreateInfo image_view_create_info = {
      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, // sType
      NULL,                                     // pNext
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
      &image_view_create_info,
      NULL,
      &window->depth_stencil.view));
}

static inline void create_render_pass(re_window_t *window) {
  window->render_target.sample_count = VK_SAMPLE_COUNT_1_BIT;

  VkAttachmentDescription attachment_descriptions[] = {
      // Resolved color attachment
      (VkAttachmentDescription){
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
      (VkAttachmentDescription){
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

  VkAttachmentReference color_attachment_reference = {
      0,                                        // attachment
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // layout
  };

  VkAttachmentReference depth_attachment_reference = {
      1,                                                // attachment
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, // layout
  };

  VkSubpassDescription subpass_description = {
      0,                               // flags
      VK_PIPELINE_BIND_POINT_GRAPHICS, // pipelineBindPoint
      0,                               // inputAttachmentCount
      NULL,                            // pInputAttachments
      1,                               // colorAttachmentCount
      &color_attachment_reference,     // pColorAttachments
      NULL,                            // pResolveAttachments
      &depth_attachment_reference,     // pDepthStencilAttachment
      0,                               // preserveAttachmentCount
      NULL,                            // pPreserveAttachments
  };

  VkSubpassDependency dependencies[] = {
      (VkSubpassDependency){
          VK_SUBPASS_EXTERNAL,                           // srcSubpass
          0,                                             // dstSubpass
          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,          // srcStageMask
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // dstStageMask
          VK_ACCESS_MEMORY_READ_BIT,                     // srcAccessMask
          VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
              VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, // dstAccessMask
          VK_DEPENDENCY_BY_REGION_BIT,              // dependencyFlags
      },
      (VkSubpassDependency){
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

  VkRenderPassCreateInfo render_pass_create_info = {
      VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,     // sType
      NULL,                                          // pNext
      0,                                             // flags
      (uint32_t)ARRAY_SIZE(attachment_descriptions), // attachmentCount
      attachment_descriptions,                       // pAttachments
      1,                                             // subpassCount
      &subpass_description,                          // pSubpasses
      (uint32_t)ARRAY_SIZE(dependencies),            // dependencyCount
      dependencies,                                  // pDependencies
  };

  VK_CHECK(vkCreateRenderPass(
      g_ctx.device,
      &render_pass_create_info,
      NULL,
      &window->render_target.render_pass));
}

static inline void regen_framebuffer(
    re_window_t *window,
    VkFramebuffer *framebuffer,
    VkImageView *swapchain_image_view) {
  vkDestroyFramebuffer(g_ctx.device, *framebuffer, NULL);

  VkImageView attachments[] = {
      *swapchain_image_view,
      window->depth_stencil.view,
  };

  VkFramebufferCreateInfo create_info = {
      VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, // sType
      NULL,                                      // pNext
      0,                                         // flags
      window->render_target.render_pass,         // renderPass
      (uint32_t)ARRAY_SIZE(attachments),         // attachmentCount
      attachments,                               // pAttachments
      window->swapchain_extent.width,            // width
      window->swapchain_extent.height,           // height
      1,                                         // layers
  };

  VK_CHECK(vkCreateFramebuffer(g_ctx.device, &create_info, NULL, framebuffer));
}

static inline void destroy_resizables(re_window_t *window) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  for (uint32_t i = 0; i < ARRAY_SIZE(window->frame_resources); i++) {
    vkFreeCommandBuffers(
        g_ctx.device,
        g_ctx.graphics_command_pool,
        1,
        &window->frame_resources[i].command_buffer);
  }

  if (window->depth_stencil.image) {
    vkDestroyImageView(g_ctx.device, window->depth_stencil.view, NULL);
    vmaDestroyImage(
        g_ctx.gpu_allocator,
        window->depth_stencil.image,
        window->depth_stencil.allocation);
    window->depth_stencil.image = VK_NULL_HANDLE;
    window->depth_stencil.allocation = VK_NULL_HANDLE;
  }

  vkDestroyRenderPass(g_ctx.device, window->render_target.render_pass, NULL);
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

void framebuffer_resize_callback(
    GLFWwindow *glfw_window, int width, int height) {
  re_window_t *window = glfwGetWindowUserPointer(glfw_window);

  update_size(window);

  if (window->framebuffer_resize_callback != NULL) {
    window->framebuffer_resize_callback(window, width, height);
  }
}

void mouse_button_callback(
    GLFWwindow *glfw_window, int button, int action, int mods) {
  re_window_t *window = glfwGetWindowUserPointer(glfw_window);

  if (window->mouse_button_callback != NULL) {
    window->mouse_button_callback(window, button, action, mods);
  }
}

void scroll_callback(GLFWwindow *glfw_window, double xoffset, double yoffset) {
  re_window_t *window = glfwGetWindowUserPointer(glfw_window);

  if (window->scroll_callback != NULL) {
    window->scroll_callback(window, xoffset, yoffset);
  }
}

void key_callback(
    GLFWwindow *glfw_window, int key, int scancode, int action, int mods) {
  re_window_t *window = glfwGetWindowUserPointer(glfw_window);

  if (window->key_callback != NULL) {
    window->key_callback(window, key, scancode, action, mods);
  }
}

void char_callback(GLFWwindow *glfw_window, unsigned int c) {
  re_window_t *window = glfwGetWindowUserPointer(glfw_window);

  if (window->char_callback != NULL) {
    window->char_callback(window, c);
  }
}

bool re_window_init(
    re_window_t *window, const char *title, uint32_t width, uint32_t height) {
  window->clear_color = (vec4_t){0.0, 0.0, 0.0, 1.0};
  window->delta_time = 0.0f;
  window->time_before = 0.0f;
  window->user_ptr = NULL;
  window->framebuffer_resize_callback = NULL;
  window->mouse_button_callback = NULL;
  window->scroll_callback = NULL;
  window->key_callback = NULL;
  window->char_callback = NULL;

  window->swapchain = VK_NULL_HANDLE;
  window->surface = VK_NULL_HANDLE;
  window->swapchain_image_count = 0;
  window->swapchain_images = NULL;
  window->swapchain_image_views = NULL;

  window->current_frame = 0;
  // Index of the current swapchain image
  window->current_image_index = 0;

  for (uint32_t i = 0; i < ARRAY_SIZE(window->frame_resources); i++) {
    window->frame_resources[i].framebuffer = VK_NULL_HANDLE;
    window->frame_resources[i].command_buffer = VK_NULL_HANDLE;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window->glfw_window = glfwCreateWindow(width, height, title, NULL, NULL);

  glfwSetWindowUserPointer(window->glfw_window, window);

  // Set callbacks
  glfwSetFramebufferSizeCallback(
      window->glfw_window, framebuffer_resize_callback);
  glfwSetMouseButtonCallback(window->glfw_window, mouse_button_callback);
  glfwSetScrollCallback(window->glfw_window, scroll_callback);
  glfwSetKeyCallback(window->glfw_window, key_callback);
  glfwSetCharCallback(window->glfw_window, char_callback);

  assert(window->glfw_window != NULL);

  VK_CHECK(create_vulkan_surface(window));

  VkBool32 supported;
  vkGetPhysicalDeviceSurfaceSupportKHR(
      g_ctx.physical_device,
      g_ctx.present_queue_family_index,
      window->surface,
      &supported);
  if (!supported) {
    return false;
  }

  window->max_samples = re_context_get_max_sample_count();

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
  glfwGetWindowSize(window->glfw_window, &iwidth, &iheight);
  *width = (uint32_t)iwidth;
  *height = (uint32_t)iheight;
}

void re_window_destroy(re_window_t *window) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  destroy_resizables(window);

  for (uint32_t i = 0; i < window->swapchain_image_count; i++) {
    vkDestroyImageView(g_ctx.device, window->swapchain_image_views[i], NULL);
  }

  vkDestroySwapchainKHR(g_ctx.device, window->swapchain, NULL);

  for (uint32_t i = 0; i < ARRAY_SIZE(window->frame_resources); i++) {
    vkDestroyFramebuffer(
        g_ctx.device, window->frame_resources[i].framebuffer, NULL);
    vkDestroySemaphore(
        g_ctx.device,
        window->frame_resources[i].image_available_semaphore,
        NULL);
    vkDestroySemaphore(
        g_ctx.device,
        window->frame_resources[i].rendering_finished_semaphore,
        NULL);
    vkDestroyFence(g_ctx.device, window->frame_resources[i].fence, NULL);
  }

  vkDestroySurfaceKHR(g_ctx.instance, window->surface, NULL);

  glfwDestroyWindow(window->glfw_window);

  free(window->swapchain_images);
  free(window->swapchain_image_views);
}

void re_window_poll_events(re_window_t *window) { glfwPollEvents(); }

bool re_window_should_close(re_window_t *window) {
  return glfwWindowShouldClose(window->glfw_window);
}

void re_window_begin_frame(re_window_t *window) {
  window->time_before = glfwGetTime();

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

  VkImageSubresourceRange image_subresource_range = {
      VK_IMAGE_ASPECT_COLOR_BIT, // aspectMask
      0,                         // baseMipLevel
      1,                         // levelCount
      0,                         // baseArrayLayer
      1,                         // layerCount
  };

  regen_framebuffer(
      window,
      &window->frame_resources[window->current_frame].framebuffer,
      &window->swapchain_image_views[window->current_image_index]);

  VkCommandBufferBeginInfo begin_info = {0};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  begin_info.pInheritanceInfo = NULL;

  VkCommandBuffer command_buffer =
      window->frame_resources[window->current_frame].command_buffer;

  VK_CHECK(vkBeginCommandBuffer(command_buffer, &begin_info));

  if (g_ctx.present_queue != g_ctx.graphics_queue) {
    VkImageMemoryBarrier barrier_from_present_to_draw = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,   // sType
        NULL,                                     // pNext
        VK_ACCESS_MEMORY_READ_BIT,                // srcAccessMask
        VK_ACCESS_MEMORY_READ_BIT,                // dstAccessMask
        VK_IMAGE_LAYOUT_UNDEFINED,                // oldLayout
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // newLayout
        g_ctx.present_queue_family_index,         // srcQueueFamilyIndex
        g_ctx.graphics_queue_family_index,        // dstQueueFamilyIndex
        window->swapchain_images[window->current_image_index], // image
        image_subresource_range, // subresourceRange
    };

    vkCmdPipelineBarrier(
        command_buffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0,
        0,
        NULL,
        0,
        NULL,
        1,
        &barrier_from_present_to_draw);
  }
}

void re_window_end_frame(re_window_t *window) {
  VkCommandBuffer command_buffer =
      window->frame_resources[window->current_frame].command_buffer;

  VkImageSubresourceRange image_subresource_range = {
      VK_IMAGE_ASPECT_COLOR_BIT, // aspectMask
      0,                         // baseMipLevel
      1,                         // levelCount
      0,                         // baseArrayLayer
      1,                         // layerCount
  };

  if (g_ctx.present_queue != g_ctx.graphics_queue) {
    VkImageMemoryBarrier barrierFromDrawToPresent = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,   // sType
        NULL,                                     // pNext
        VK_ACCESS_MEMORY_READ_BIT,                // srcAccessMask
        VK_ACCESS_MEMORY_READ_BIT,                // dstAccessMask
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // oldLayout
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,          // newLayout
        g_ctx.graphics_queue_family_index,        // srcQueueFamilyIndex
        g_ctx.present_queue_family_index,         // dstQueueFamilyIndex
        window->swapchain_images[window->current_image_index], // image
        image_subresource_range, // subresourceRange
    };

    vkCmdPipelineBarrier(
        command_buffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0,
        0,
        NULL,
        0,
        NULL,
        1,
        &barrierFromDrawToPresent);
  }

  VK_CHECK(vkEndCommandBuffer(command_buffer));

  // Present
  VkPipelineStageFlags wait_dst_stage_mask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  VkSubmitInfo submit_info = {
      VK_STRUCTURE_TYPE_SUBMIT_INFO, // sType
      NULL,                          // pNext
      1,                             // waitSemaphoreCount
      &window->frame_resources[window->current_frame]
           .image_available_semaphore, // pWaitSemaphores
      &wait_dst_stage_mask,            // pWaitDstStageMask
      1,                               // commandBufferCount
      &command_buffer,                 // pCommandBuffers
      1,                               // signalSemaphoreCount
      &window->frame_resources[window->current_frame]
           .rendering_finished_semaphore, // pSignalSemaphores
  };

  mtx_lock(&g_ctx.queue_mutex);
  vkQueueSubmit(
      g_ctx.graphics_queue,
      1,
      &submit_info,
      window->frame_resources[window->current_frame].fence);

  VkPresentInfoKHR presentInfo = {
      VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      NULL, // pNext
      1,    // waitSemaphoreCount
      &window->frame_resources[window->current_frame]
           .rendering_finished_semaphore, // pWaitSemaphores
      1,                                  // swapchainCount
      &window->swapchain,                 // pSwapchains
      &window->current_image_index,       // pImageIndices
      NULL,                               // pResults
  };

  VkResult result = vkQueuePresentKHR(g_ctx.present_queue, &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    update_size(window);
  } else {
    assert(result == VK_SUCCESS);
  }

  mtx_unlock(&g_ctx.queue_mutex);

  window->current_frame = (window->current_frame + 1) % RE_MAX_FRAMES_IN_FLIGHT;

  window->delta_time = glfwGetTime() - window->time_before;
}

void re_window_begin_render_pass(re_window_t *window) {
  VkCommandBuffer command_buffer =
      window->frame_resources[window->current_frame].command_buffer;

  VkClearValue clear_values[2] = {0};
  clear_values[0].color = (VkClearColorValue){{
      window->clear_color.x,
      window->clear_color.y,
      window->clear_color.z,
      window->clear_color.w,
  }};
  clear_values[1].depthStencil = (VkClearDepthStencilValue){1.0f, 0};

  VkRenderPassBeginInfo render_pass_begin_info = {
      VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,                   // sType
      NULL,                                                       // pNext
      window->render_target.render_pass,                          // renderPass
      window->frame_resources[window->current_frame].framebuffer, // framebuffer
      {{0, 0}, window->swapchain_extent},                         // renderArea
      ARRAY_SIZE(clear_values), // clearValueCount
      clear_values,             // pClearValues
  };

  vkCmdBeginRenderPass(
      command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport = {
      0.0f,                                   // x
      0.0f,                                   // y
      (float)window->swapchain_extent.width,  // width
      (float)window->swapchain_extent.height, // height
      0.0f,                                   // minDepth
      1.0f,                                   // maxDepth
  };

  vkCmdSetViewport(command_buffer, 0, 1, &viewport);

  VkRect2D scissor = {{0, 0}, window->swapchain_extent};

  vkCmdSetScissor(command_buffer, 0, 1, &scissor);
}

void re_window_end_render_pass(re_window_t *window) {
  VkCommandBuffer command_buffer =
      window->frame_resources[window->current_frame].command_buffer;

  // End
  vkCmdEndRenderPass(command_buffer);
}

void re_window_set_input_mode(const re_window_t *window, int mode, int value) {
  glfwSetInputMode(window->glfw_window, mode, value);
}

int re_window_get_input_mode(const re_window_t *window, int mode) {
  return glfwGetInputMode(window->glfw_window, mode);
}

void re_window_get_cursor_pos(const re_window_t *window, double *x, double *y) {
  glfwGetCursorPos(window->glfw_window, x, y);
}

void re_window_set_cursor_pos(re_window_t *window, double x, double y) {
  glfwSetCursorPos(window->glfw_window, x, y);
}

bool re_window_is_mouse_left_pressed(const re_window_t *window) {
  return glfwGetMouseButton(window->glfw_window, GLFW_MOUSE_BUTTON_LEFT) ==
         GLFW_PRESS;
}

bool re_window_is_mouse_right_pressed(const re_window_t *window) {
  return glfwGetMouseButton(window->glfw_window, GLFW_MOUSE_BUTTON_RIGHT) ==
         GLFW_PRESS;
}

bool re_window_is_key_pressed(const re_window_t *window, int key) {
  return glfwGetKey(window->glfw_window, key) == GLFW_PRESS;
}

VkCommandBuffer
re_window_get_current_command_buffer(const re_window_t *window) {
  return window->frame_resources[window->current_frame].command_buffer;
}
