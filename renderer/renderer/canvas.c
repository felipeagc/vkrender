#include "canvas.h"
#include "context.h"
#include "util.h"
#include "window.h"
#include <fstd_util.h>

static inline void create_image(
    VkImage *image,
    VmaAllocation *image_allocation,
    VkImageView *image_view,
    uint32_t width,
    uint32_t height,
    VkFormat format,
    VkSampleCountFlags sample_count) {
  VkImageCreateInfo image_create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = format,
      .extent = {width, height, 1},
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = sample_count,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
               VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = NULL,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };

  VmaAllocationCreateInfo alloc_info = {0};
  alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  alloc_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

  VK_CHECK(vmaCreateImage(
      g_ctx.gpu_allocator,
      &image_create_info,
      &alloc_info,
      image,
      image_allocation,
      NULL));

  VkImageViewCreateInfo image_view_create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .image = *image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = format,
      .components =
          {
              .r = VK_COMPONENT_SWIZZLE_IDENTITY,
              .g = VK_COMPONENT_SWIZZLE_IDENTITY,
              .b = VK_COMPONENT_SWIZZLE_IDENTITY,
              .a = VK_COMPONENT_SWIZZLE_IDENTITY,
          },
      .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
      }};

  VK_CHECK(vkCreateImageView(
      g_ctx.device, &image_view_create_info, NULL, image_view));
}

static inline void destroy_images(re_canvas_t *canvas) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  if (canvas->color.image_view != VK_NULL_HANDLE) {
    vkDestroyImageView(g_ctx.device, canvas->color.image_view, NULL);
  }

  if (canvas->color.image != VK_NULL_HANDLE) {
    vmaDestroyImage(
        g_ctx.gpu_allocator, canvas->color.image, canvas->color.allocation);
  }

  if (canvas->resolve.image_view != VK_NULL_HANDLE) {
    vkDestroyImageView(g_ctx.device, canvas->resolve.image_view, NULL);
  }

  if (canvas->resolve.image != VK_NULL_HANDLE) {
    vmaDestroyImage(
        g_ctx.gpu_allocator, canvas->resolve.image, canvas->resolve.allocation);
  }

  if (canvas->depth.image != VK_NULL_HANDLE) {
    vmaDestroyImage(
        g_ctx.gpu_allocator, canvas->depth.image, canvas->depth.allocation);
  }

  if (canvas->depth.image_view != VK_NULL_HANDLE) {
    vkDestroyImageView(g_ctx.device, canvas->depth.image_view, NULL);
  }

  canvas->color.image = VK_NULL_HANDLE;
  canvas->color.allocation = VK_NULL_HANDLE;
  canvas->color.image_view = VK_NULL_HANDLE;

  canvas->resolve.image = VK_NULL_HANDLE;
  canvas->resolve.allocation = VK_NULL_HANDLE;
  canvas->resolve.image_view = VK_NULL_HANDLE;

  canvas->depth.image = VK_NULL_HANDLE;
  canvas->depth.allocation = VK_NULL_HANDLE;
  canvas->depth.image_view = VK_NULL_HANDLE;
}

static inline void create_sampler(VkSampler *sampler) {
  VkSamplerCreateInfo sampler_create_info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
      .mipLodBias = 0.0f,
      .anisotropyEnable = VK_FALSE,
      .maxAnisotropy = 1.0f,
      .compareEnable = VK_FALSE,
      .compareOp = VK_COMPARE_OP_NEVER,
      .minLod = 0.0f,
      .maxLod = 0.0f,
      .borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
      .unnormalizedCoordinates = VK_FALSE,
  };

  VK_CHECK(vkCreateSampler(g_ctx.device, &sampler_create_info, NULL, sampler));
}

static inline void destroy_sampler(re_canvas_t *canvas) {
  if (canvas->sampler != VK_NULL_HANDLE) {
    vkDestroySampler(g_ctx.device, canvas->sampler, NULL);
  }
}

static inline void create_depth_target(re_canvas_t *canvas) {
  VkImageCreateInfo image_create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = canvas->depth_format,
      .extent = {canvas->render_target.width, canvas->render_target.height, 1},
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = canvas->render_target.sample_count,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
               VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = NULL,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };

  VmaAllocationCreateInfo alloc_info = {0};
  alloc_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
  alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  VK_CHECK(vmaCreateImage(
      g_ctx.gpu_allocator,
      &image_create_info,
      &alloc_info,
      &canvas->depth.image,
      &canvas->depth.allocation,
      NULL));

  VkImageViewCreateInfo image_view_create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .image = canvas->depth.image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = canvas->depth_format,
      .components =
          {
              .r = VK_COMPONENT_SWIZZLE_IDENTITY,
              .g = VK_COMPONENT_SWIZZLE_IDENTITY,
              .b = VK_COMPONENT_SWIZZLE_IDENTITY,
              .a = VK_COMPONENT_SWIZZLE_IDENTITY,
          },
      .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
      }};

  if (canvas->depth_format >= VK_FORMAT_D16_UNORM_S8_UINT) {
    image_view_create_info.subresourceRange.aspectMask |=
        VK_IMAGE_ASPECT_STENCIL_BIT;
  }

  VK_CHECK(vkCreateImageView(
      g_ctx.device, &image_view_create_info, NULL, &canvas->depth.image_view));
}

static inline void create_framebuffers(re_canvas_t *canvas) {
  VkImageView attachments[] = {
      canvas->color.image_view,
      canvas->depth.image_view,
      canvas->resolve.image_view,
  };

  VkFramebufferCreateInfo create_info = {
      VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, // sType
      NULL,                                      // pNext
      0,                                         // flags
      canvas->render_target.render_pass,         // renderPass
      (uint32_t)ARRAY_SIZE(attachments),         // attachmentCount
      attachments,                               // pAttachments
      canvas->render_target.width,               // width
      canvas->render_target.height,              // height
      1,                                         // layers
  };

  if (canvas->render_target.sample_count == VK_SAMPLE_COUNT_1_BIT) {
    create_info.attachmentCount -= 1;
  }

  VK_CHECK(vkCreateFramebuffer(
      g_ctx.device, &create_info, NULL, &canvas->framebuffer));
}

static inline void destroy_framebuffers(re_canvas_t *canvas) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  if (canvas->framebuffer != VK_NULL_HANDLE) {
    vkDestroyFramebuffer(g_ctx.device, canvas->framebuffer, NULL);
  }
}

static inline void create_render_pass(re_canvas_t *canvas) {
  VkAttachmentDescription attachment_descriptions[] = {
      // Multisampled color attachment
      (VkAttachmentDescription){
          0,                                        // flags
          canvas->color_format,                     // format
          canvas->render_target.sample_count,       // samples
          VK_ATTACHMENT_LOAD_OP_CLEAR,              // loadOp
          VK_ATTACHMENT_STORE_OP_STORE,             // storeOp
          VK_ATTACHMENT_LOAD_OP_DONT_CARE,          // stencilLoadOp
          VK_ATTACHMENT_STORE_OP_DONT_CARE,         // stencilStoreOp
          VK_IMAGE_LAYOUT_UNDEFINED,                // initialLayout
          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, // finalLayout
      },

      // Multisampled depth attachment
      (VkAttachmentDescription){
          0,                                                // flags
          canvas->depth_format,                             // format
          canvas->render_target.sample_count,               // samples
          VK_ATTACHMENT_LOAD_OP_CLEAR,                      // loadOp
          VK_ATTACHMENT_STORE_OP_DONT_CARE,                 // storeOp
          VK_ATTACHMENT_LOAD_OP_DONT_CARE,                  // stencilLoadOp
          VK_ATTACHMENT_STORE_OP_DONT_CARE,                 // stencilStoreOp
          VK_IMAGE_LAYOUT_UNDEFINED,                        // initialLayout
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, // finalLayout
      },

      // Resolved color attachment
      (VkAttachmentDescription){
          0,                                        // flags
          canvas->color_format,                     // format
          VK_SAMPLE_COUNT_1_BIT,                    // samples
          VK_ATTACHMENT_LOAD_OP_CLEAR,              // loadOp
          VK_ATTACHMENT_STORE_OP_STORE,             // storeOp
          VK_ATTACHMENT_LOAD_OP_DONT_CARE,          // stencilLoadOp
          VK_ATTACHMENT_STORE_OP_DONT_CARE,         // stencilStoreOp
          VK_IMAGE_LAYOUT_UNDEFINED,                // initialLayout
          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, // finalLayout
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

  VkAttachmentReference resolve_color_attachment_reference = {
      2,                                        // attachment
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // layout
  };

  VkSubpassDescription subpass_description = {
      .flags = 0,
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .inputAttachmentCount = 0,
      .pInputAttachments = NULL,
      .colorAttachmentCount = 1,
      .pColorAttachments = &color_attachment_reference,
      .pResolveAttachments = &resolve_color_attachment_reference,
      .pDepthStencilAttachment = &depth_attachment_reference,
      .preserveAttachmentCount = 0,
      .pPreserveAttachments = NULL,
  };

  if (canvas->render_target.sample_count == VK_SAMPLE_COUNT_1_BIT) {
    subpass_description.pResolveAttachments = NULL;
  }

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

  if (canvas->render_target.sample_count == VK_SAMPLE_COUNT_1_BIT) {
    render_pass_create_info.attachmentCount -= 1;
  }

  VK_CHECK(vkCreateRenderPass(
      g_ctx.device,
      &render_pass_create_info,
      NULL,
      &canvas->render_target.render_pass));
}

static inline void destroy_render_pass(re_canvas_t *canvas) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));
  if (canvas->render_target.render_pass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(g_ctx.device, canvas->render_target.render_pass, NULL);
  }
}

void re_canvas_init(re_canvas_t *canvas, re_canvas_options_t *options) {
  assert(options->width > 0);
  assert(options->height > 0);
  canvas->render_target.width = options->width;
  canvas->render_target.height = options->height;

  if (options->sample_count == 0) {
    options->sample_count = VK_SAMPLE_COUNT_1_BIT;
  }
  canvas->render_target.sample_count = options->sample_count;

  if (options->color_format == 0) {
    options->color_format = VK_FORMAT_R8G8B8A8_UNORM;
  }
  canvas->color_format = options->color_format;

  canvas->clear_color = options->clear_color;

  bool res = re_ctx_get_supported_depth_format(&canvas->depth_format);
  assert(res);

  create_image(
      &canvas->color.image,
      &canvas->color.allocation,
      &canvas->color.image_view,
      canvas->render_target.width,
      canvas->render_target.height,
      canvas->color_format,
      canvas->render_target.sample_count);

  create_image(
      &canvas->resolve.image,
      &canvas->resolve.allocation,
      &canvas->resolve.image_view,
      canvas->render_target.width,
      canvas->render_target.height,
      canvas->color_format,
      VK_SAMPLE_COUNT_1_BIT);

  create_sampler(&canvas->sampler);
  create_depth_target(canvas);
  create_render_pass(canvas);
  create_framebuffers(canvas);
}

void re_canvas_begin(re_canvas_t *canvas, re_cmd_buffer_t *cmd_buffer) {
  // @TODO: make this customizable
  VkClearValue clear_values[] = {
      {.color = canvas->clear_color},
      {.depthStencil = {1.0f, 0}},
      {.color = canvas->clear_color},
  };

  VkRenderPassBeginInfo render_pass_begin_info = {
      VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, // sType
      NULL,                                     // pNext
      canvas->render_target.render_pass,        // renderPass
      canvas->framebuffer,                      // framebuffer
      {{0, 0},
       {canvas->render_target.width,
        canvas->render_target.height}}, // renderArea
      ARRAY_SIZE(clear_values),         // clearValueCount
      clear_values,                     // pClearValues
  };

  if (canvas->render_target.sample_count == VK_SAMPLE_COUNT_1_BIT) {
    render_pass_begin_info.clearValueCount -= 1;
  }

  vkCmdBeginRenderPass(
      cmd_buffer->cmd_buffer,
      &render_pass_begin_info,
      VK_SUBPASS_CONTENTS_INLINE);

  re_cmd_set_viewport(
      cmd_buffer,
      &(re_viewport_t){
          .x = 0.0f,
          .y = 0.0f,
          .width = (float)canvas->render_target.width,
          .height = (float)canvas->render_target.height,
          .min_depth = 0.0f,
          .max_depth = 1.0f,
      });

  re_cmd_set_scissor(
      cmd_buffer,
      &(re_rect_2d_t){
          .offset = {0, 0},
          .extent = {canvas->render_target.width, canvas->render_target.height},
      });
}

void re_canvas_end(re_canvas_t *canvas, re_cmd_buffer_t *cmd_buffer) {
  vkCmdEndRenderPass(cmd_buffer->cmd_buffer);
}

void re_canvas_draw(
    re_canvas_t *canvas, re_cmd_buffer_t *cmd_buffer, re_pipeline_t *pipeline) {
  re_cmd_bind_pipeline(cmd_buffer, pipeline);

  VkDescriptorImageInfo descriptor = {
      .sampler = canvas->sampler,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };

  if (canvas->render_target.sample_count == VK_SAMPLE_COUNT_1_BIT) {
    descriptor.imageView = canvas->color.image_view;
  } else {
    descriptor.imageView = canvas->resolve.image_view;
  }

  re_cmd_bind_descriptor(
      cmd_buffer, 0, 0, (re_descriptor_info_t){.image = descriptor});

  re_cmd_bind_descriptor_set(cmd_buffer, pipeline, 0);

  re_cmd_draw(cmd_buffer, 3, 1, 0, 0);
}

void re_canvas_resize(
    re_canvas_t *canvas, const uint32_t width, const uint32_t height) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));
  canvas->render_target.width = width;
  canvas->render_target.height = height;

  destroy_framebuffers(canvas);
  destroy_render_pass(canvas);
  destroy_images(canvas);

  create_image(
      &canvas->color.image,
      &canvas->color.allocation,
      &canvas->color.image_view,
      canvas->render_target.width,
      canvas->render_target.height,
      canvas->color_format,
      canvas->render_target.sample_count);

  create_image(
      &canvas->resolve.image,
      &canvas->resolve.allocation,
      &canvas->resolve.image_view,
      canvas->render_target.width,
      canvas->render_target.height,
      canvas->color_format,
      VK_SAMPLE_COUNT_1_BIT);

  create_depth_target(canvas);
  create_render_pass(canvas);
  create_framebuffers(canvas);
}

void re_canvas_destroy(re_canvas_t *canvas) {
  destroy_framebuffers(canvas);
  destroy_render_pass(canvas);
  destroy_images(canvas);
  destroy_sampler(canvas);
}
