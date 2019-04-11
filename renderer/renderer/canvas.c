#include "canvas.h"
#include "context.h"
#include "util.h"
#include "window.h"
#include <fstd_util.h>

static inline void create_color_target(re_canvas_t *canvas) {
  for (size_t i = 0; i < ARRAY_SIZE(canvas->resources); i++) {
    struct re_canvas_resource_t *resource = &canvas->resources[i];

    VkImageCreateInfo image_create_info = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        NULL,
        0,                    // flags
        VK_IMAGE_TYPE_2D,     // imageType
        canvas->color_format, // format
        {
            canvas->width,                  // width
            canvas->height,                 // height
            1,                              // depth
        },                                  // extent
        1,                                  // mipLevels
        1,                                  // arrayLayers
        canvas->render_target.sample_count, // samples
        VK_IMAGE_TILING_OPTIMAL,            // tiling
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, // usage
        VK_SHARING_MODE_EXCLUSIVE,               // sharingMode
        0,                                       // queueFamilyIndexCount
        NULL,                                    // pQueueFamilyIndices
        VK_IMAGE_LAYOUT_UNDEFINED,               // initialLayout
    };

    VmaAllocationCreateInfo image_alloc_create_info = {0};
    image_alloc_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VK_CHECK(vmaCreateImage(
        g_ctx.gpu_allocator,
        &image_create_info,
        &image_alloc_create_info,
        &resource->color.image,
        &resource->color.allocation,
        NULL));

    VkImageViewCreateInfo image_view_create_info = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        NULL,
        0,                     // flags
        resource->color.image, // image
        VK_IMAGE_VIEW_TYPE_2D, // viewType
        canvas->color_format,  // format
        {
            VK_COMPONENT_SWIZZLE_IDENTITY, // r
            VK_COMPONENT_SWIZZLE_IDENTITY, // g
            VK_COMPONENT_SWIZZLE_IDENTITY, // b
            VK_COMPONENT_SWIZZLE_IDENTITY, // a
        },                                 // components
        {
            VK_IMAGE_ASPECT_COLOR_BIT, // aspectMask
            0,                         // baseMipLevel
            1,                         // levelCount
            0,                         // baseArrayLayer
            1,                         // layerCount
        },                             // subresourceRange
    };

    VK_CHECK(vkCreateImageView(
        g_ctx.device,
        &image_view_create_info,
        NULL,
        &resource->color.image_view));

    VkSamplerCreateInfo sampler_create_info = {
        VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        NULL,
        0,                                       // flags
        VK_FILTER_LINEAR,                        // magFilter
        VK_FILTER_LINEAR,                        // minFilter
        VK_SAMPLER_MIPMAP_MODE_LINEAR,           // mipmapMode
        VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT, // addressModeU
        VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT, // addressModeV
        VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT, // addressModeW
        0.0f,                                    // mipLodBias
        VK_FALSE,                                // anisotropyEnable
        1.0f,                                    // maxAnisotropy
        VK_FALSE,                                // compareEnable
        VK_COMPARE_OP_NEVER,                     // compareOp
        0.0f,                                    // minLod
        0.0f,                                    // maxLod
        VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK, // borderColor
        VK_FALSE,                                // unnormalizedCoordinates
    };

    VK_CHECK(vkCreateSampler(
        g_ctx.device, &sampler_create_info, NULL, &resource->color.sampler));
  }
}

static inline void destroy_color_target(re_canvas_t *canvas) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));
  for (size_t i = 0; i < ARRAY_SIZE(canvas->resources); i++) {
    struct re_canvas_resource_t *resource = &canvas->resources[i];

    if (resource->color.image != VK_NULL_HANDLE) {
      vkDestroyImageView(g_ctx.device, resource->color.image_view, NULL);
    }

    if (resource->color.sampler != VK_NULL_HANDLE) {
      vkDestroySampler(g_ctx.device, resource->color.sampler, NULL);
    }

    if (resource->color.image != VK_NULL_HANDLE) {
      vmaDestroyImage(
          g_ctx.gpu_allocator,
          resource->color.image,
          resource->color.allocation);
    }

    resource->color.image = VK_NULL_HANDLE;
    resource->color.allocation = VK_NULL_HANDLE;
    resource->color.image_view = VK_NULL_HANDLE;
    resource->color.sampler = VK_NULL_HANDLE;
  }
}

static inline void create_depth_target(re_canvas_t *canvas) {
  for (size_t i = 0; i < ARRAY_SIZE(canvas->resources); i++) {
    struct re_canvas_resource_t *resource = &canvas->resources[i];

    VkImageCreateInfo image_create_info = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, // sType
        NULL,                                // pNext
        0,                                   // flags
        VK_IMAGE_TYPE_2D,                    // imageType
        canvas->depth_format,                // format
        {
            canvas->width,                  // width
            canvas->height,                 // height
            1,                              // depth
        },                                  // extent
        1,                                  // mipLevels
        1,                                  // arrayLayers
        canvas->render_target.sample_count, // samples
        VK_IMAGE_TILING_OPTIMAL,            // tiling
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT, // usage
        VK_SHARING_MODE_EXCLUSIVE,      // sharingMode
        0,                              // queueFamiylIndexCount
        NULL,                           // pQueueFamilyIndices
        VK_IMAGE_LAYOUT_UNDEFINED,      // initialLayout
    };

    VmaAllocationCreateInfo alloc_info = {0};
    alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VK_CHECK(vmaCreateImage(
        g_ctx.gpu_allocator,
        &image_create_info,
        &alloc_info,
        &resource->depth.image,
        &resource->depth.allocation,
        NULL));

    VkImageViewCreateInfo image_view_create_info = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, // sType
        NULL,                                     // pNext
        0,                                        // flags
        resource->depth.image,                    // image
        VK_IMAGE_VIEW_TYPE_2D,                    // viewType
        canvas->depth_format,                     // format
        {
            VK_COMPONENT_SWIZZLE_IDENTITY, // r
            VK_COMPONENT_SWIZZLE_IDENTITY, // g
            VK_COMPONENT_SWIZZLE_IDENTITY, // b
            VK_COMPONENT_SWIZZLE_IDENTITY, // a
        },                                 // components
        {
            VK_IMAGE_ASPECT_DEPTH_BIT |
                VK_IMAGE_ASPECT_STENCIL_BIT, // aspectMask
            0,                               // baseMipLevel
            1,                               // levelCount
            0,                               // baseArrayLayer
            1,                               // layerCount
        },                                   // subresourceRange
    };

    VK_CHECK(vkCreateImageView(
        g_ctx.device,
        &image_view_create_info,
        NULL,
        &resource->depth.image_view));
  }
}

static inline void destroy_depth_target(re_canvas_t *canvas) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  for (size_t i = 0; i < ARRAY_SIZE(canvas->resources); i++) {
    struct re_canvas_resource_t *resource = &canvas->resources[i];

    if (resource->depth.image != VK_NULL_HANDLE) {
      vmaDestroyImage(
          g_ctx.gpu_allocator,
          resource->depth.image,
          resource->depth.allocation);
    }

    if (resource->depth.image_view != VK_NULL_HANDLE) {
      vkDestroyImageView(g_ctx.device, resource->depth.image_view, NULL);
    }
  }
}

static inline void create_descriptor_sets(re_canvas_t *canvas) {
  for (size_t i = 0; i < ARRAY_SIZE(canvas->resources); i++) {
    {
      VkDescriptorSetAllocateInfo alloc_info = {0};
      alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
      alloc_info.pNext = NULL;
      alloc_info.descriptorPool = g_ctx.descriptor_pool;
      alloc_info.descriptorSetCount = 1;
      alloc_info.pSetLayouts = &g_ctx.canvas_descriptor_set_layout;

      VK_CHECK(vkAllocateDescriptorSets(
          g_ctx.device, &alloc_info, &canvas->resources[i].descriptor_set));
    }

    VkDescriptorImageInfo descriptor = {
        canvas->resources[i].color.sampler,
        canvas->resources[i].color.image_view,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet descriptor_writes[] = {
        (VkWriteDescriptorSet){
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            NULL,
            canvas->resources[i].descriptor_set,       // dstSet
            0,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &descriptor,                               // pImageInfo
            NULL,                                      // pBufferInfo
            NULL,                                      // pTexelBufferView
        },
    };

    vkUpdateDescriptorSets(
        g_ctx.device,
        ARRAY_SIZE(descriptor_writes),
        descriptor_writes,
        0,
        NULL);
  }
}

static inline void destroy_descriptor_sets(re_canvas_t *canvas) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  for (size_t i = 0; i < ARRAY_SIZE(canvas->resources); i++) {
    vkFreeDescriptorSets(
        g_ctx.device,
        g_ctx.descriptor_pool,
        1,
        &canvas->resources[i].descriptor_set);
  }
}

static inline void create_framebuffers(re_canvas_t *canvas) {
  for (size_t i = 0; i < ARRAY_SIZE(canvas->resources); i++) {
    struct re_canvas_resource_t *resource = &canvas->resources[i];

    VkImageView attachments[] = {
        resource->color.image_view,
        resource->depth.image_view,
    };

    VkFramebufferCreateInfo create_info = {
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, // sType
        NULL,                                      // pNext
        0,                                         // flags
        canvas->render_target.render_pass,         // renderPass
        (uint32_t)ARRAY_SIZE(attachments),         // attachmentCount
        attachments,                               // pAttachments
        canvas->width,                             // width
        canvas->height,                            // height
        1,                                         // layers
    };

    VK_CHECK(vkCreateFramebuffer(
        g_ctx.device, &create_info, NULL, &resource->framebuffer));
  }
}

static inline void destroy_framebuffers(re_canvas_t *canvas) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  for (size_t i = 0; i < ARRAY_SIZE(canvas->resources); i++) {
    struct re_canvas_resource_t *resource = &canvas->resources[i];

    if (resource->framebuffer != VK_NULL_HANDLE) {
      vkDestroyFramebuffer(g_ctx.device, resource->framebuffer, NULL);
    }
  }
}

static inline void create_render_pass(re_canvas_t *canvas) {
  VkAttachmentDescription attachment_descriptions[] = {
      // Resolved color attachment
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

      // Resolved depth attachment
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
      &canvas->render_target.render_pass));
}

static inline void destroy_render_pass(re_canvas_t *canvas) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));
  if (canvas->render_target.render_pass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(g_ctx.device, canvas->render_target.render_pass, NULL);
  }
}

void re_canvas_init(
    re_canvas_t *canvas,
    const uint32_t width,
    const uint32_t height,
    const VkFormat color_format) {
  canvas->width = width;
  canvas->height = height;
  canvas->color_format = color_format;
  canvas->render_target.sample_count = VK_SAMPLE_COUNT_1_BIT;
  canvas->clear_color = (VkClearColorValue){0};

  assert(re_context_get_supported_depth_format(&canvas->depth_format));

  create_color_target(canvas);
  create_depth_target(canvas);
  create_descriptor_sets(canvas);
  create_render_pass(canvas);
  create_framebuffers(canvas);
}

void re_canvas_begin(
    re_canvas_t *canvas, const VkCommandBuffer command_buffer) {
  struct re_canvas_resource_t *resource = &canvas->resources[0];

  // @TODO: make this customizable
  VkClearValue clear_values[2] = {
      {.color = canvas->clear_color},
      {.depthStencil = {1.0f, 0}},
  };

  VkRenderPassBeginInfo render_pass_begin_info = {
      VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // sType
      NULL,                                      // pNext
      canvas->render_target.render_pass,         // renderPass
      resource->framebuffer,                     // framebuffer
      {{0, 0}, {canvas->width, canvas->height}}, // renderArea
      ARRAY_SIZE(clear_values),                  // clearValueCount
      clear_values,                              // pClearValues
  };

  vkCmdBeginRenderPass(
      command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport = {
      0.0f,                  // x
      0.0f,                  // y
      (float)canvas->width,  // width
      (float)canvas->height, // height
      0.0f,                  // minDepth
      1.0f,                  // maxDepth
  };

  vkCmdSetViewport(command_buffer, 0, 1, &viewport);

  VkRect2D scissor = {{0, 0}, {canvas->width, canvas->height}};

  vkCmdSetScissor(command_buffer, 0, 1, &scissor);
}

void re_canvas_end(re_canvas_t *canvas, const VkCommandBuffer command_buffer) {
  vkCmdEndRenderPass(command_buffer);
}

void re_canvas_draw(
    re_canvas_t *canvas,
    const VkCommandBuffer command_buffer,
    re_pipeline_t *pipeline) {
  struct re_canvas_resource_t *resource = &canvas->resources[0];

  vkCmdBindPipeline(
      command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);

  vkCmdBindDescriptorSets(
      command_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline->layout,
      0, // firstSet
      1,
      &resource->descriptor_set,
      0,
      NULL);

  vkCmdDraw(command_buffer, 3, 1, 0, 0);
}

void re_canvas_resize(
    re_canvas_t *canvas, const uint32_t width, const uint32_t height) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));
  canvas->width = width;
  canvas->height = height;

  destroy_framebuffers(canvas);
  destroy_render_pass(canvas);
  destroy_color_target(canvas);
  destroy_depth_target(canvas);
  destroy_descriptor_sets(canvas);

  create_color_target(canvas);
  create_depth_target(canvas);
  create_descriptor_sets(canvas);
  create_render_pass(canvas);
  create_framebuffers(canvas);
}

void re_canvas_destroy(re_canvas_t *canvas) {
  destroy_framebuffers(canvas);
  destroy_render_pass(canvas);
  destroy_color_target(canvas);
  destroy_depth_target(canvas);
  destroy_descriptor_sets(canvas);
}
