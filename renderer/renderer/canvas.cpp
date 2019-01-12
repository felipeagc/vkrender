#include "canvas.hpp"
#include "context.hpp"
#include "util.hpp"
#include "window.hpp"

static inline void create_color_target(re_canvas_t *canvas) {
  for (size_t i = 0; i < ARRAYSIZE(canvas->resources); i++) {
    auto &resource = canvas->resources[i];

    VkImageCreateInfo imageCreateInfo = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        nullptr,
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
        nullptr,                                 // pQueueFamilyIndices
        VK_IMAGE_LAYOUT_UNDEFINED,               // initialLayout
    };

    VmaAllocationCreateInfo imageAllocCreateInfo = {};
    imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VK_CHECK(vmaCreateImage(
        renderer::ctx().m_allocator,
        &imageCreateInfo,
        &imageAllocCreateInfo,
        &resource.color.image,
        &resource.color.allocation,
        nullptr));

    VkImageViewCreateInfo imageViewCreateInfo = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr,
        0,                     // flags
        resource.color.image,  // image
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
        renderer::ctx().m_device,
        &imageViewCreateInfo,
        nullptr,
        &resource.color.image_view));

    VkSamplerCreateInfo samplerCreateInfo = {
        VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        nullptr,
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
        renderer::ctx().m_device,
        &samplerCreateInfo,
        nullptr,
        &resource.color.sampler));
  }
}

static inline void destroy_color_target(re_canvas_t *canvas) {
  VK_CHECK(vkDeviceWaitIdle(renderer::ctx().m_device));
  for (size_t i = 0; i < ARRAYSIZE(canvas->resources); i++) {
    auto &resource = canvas->resources[i];

    if (resource.color.image != VK_NULL_HANDLE) {
      vkDestroyImageView(
          renderer::ctx().m_device, resource.color.image_view, nullptr);
    }

    if (resource.color.sampler != VK_NULL_HANDLE) {
      vkDestroySampler(
          renderer::ctx().m_device, resource.color.sampler, nullptr);
    }

    if (resource.color.image != VK_NULL_HANDLE) {
      vmaDestroyImage(
          renderer::ctx().m_allocator,
          resource.color.image,
          resource.color.allocation);
    }

    resource.color.image = VK_NULL_HANDLE;
    resource.color.allocation = VK_NULL_HANDLE;
    resource.color.image_view = VK_NULL_HANDLE;
    resource.color.sampler = VK_NULL_HANDLE;
  }
}

static inline void create_depth_target(re_canvas_t *canvas) {
  for (size_t i = 0; i < ARRAYSIZE(canvas->resources); i++) {
    auto &resource = canvas->resources[i];

    VkImageCreateInfo imageCreateInfo = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, // sType
        nullptr,                             // pNext
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
        nullptr,                        // pQueueFamilyIndices
        VK_IMAGE_LAYOUT_UNDEFINED,      // initialLayout
    };

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VK_CHECK(vmaCreateImage(
        renderer::ctx().m_allocator,
        &imageCreateInfo,
        &allocInfo,
        &resource.depth.image,
        &resource.depth.allocation,
        nullptr));

    VkImageViewCreateInfo imageViewCreateInfo = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, // sType
        nullptr,                                  // pNext
        0,                                        // flags
        resource.depth.image,                     // image
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
        renderer::ctx().m_device,
        &imageViewCreateInfo,
        nullptr,
        &resource.depth.image_view));
  }
}

static inline void destroy_depth_target(re_canvas_t *canvas) {
  VK_CHECK(vkDeviceWaitIdle(renderer::ctx().m_device));

  for (size_t i = 0; i < ARRAYSIZE(canvas->resources); i++) {
    auto &resource = canvas->resources[i];

    if (resource.depth.image != VK_NULL_HANDLE) {
      vmaDestroyImage(
          renderer::ctx().m_allocator,
          resource.depth.image,
          resource.depth.allocation);
    }

    if (resource.depth.image_view != VK_NULL_HANDLE) {
      vkDestroyImageView(
          renderer::ctx().m_device, resource.depth.image_view, nullptr);
    }
  }
}

static inline void create_descriptor_sets(re_canvas_t *canvas) {
  for (size_t i = 0; i < ARRAYSIZE(canvas->resources); i++) {
    auto &resource = canvas->resources[i];

    auto &set_layout = renderer::ctx().resource_manager.set_layouts.fullscreen;
    resource.resource_set = re_allocate_resource_set(&set_layout);

    VkDescriptorImageInfo descriptor = {
        resource.color.sampler,
        resource.color.image_view,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet descriptorWrites[] = {
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            resource.resource_set.descriptor_set,      // dstSet
            0,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &descriptor,                               // pImageInfo
            nullptr,                                   // pBufferInfo
            nullptr,                                   // pTexelBufferView
        },
    };

    vkUpdateDescriptorSets(
        renderer::ctx().m_device,
        ARRAYSIZE(descriptorWrites),
        descriptorWrites,
        0,
        nullptr);
  }
}

static inline void destroy_descriptor_sets(re_canvas_t *canvas) {
  VK_CHECK(vkDeviceWaitIdle(renderer::ctx().m_device));

  auto &set_layout = renderer::ctx().resource_manager.set_layouts.fullscreen;

  for (size_t i = 0; i < ARRAYSIZE(canvas->resources); i++) {
    auto &resource = canvas->resources[i];
    re_free_resource_set(&set_layout, &resource.resource_set);
  }
}

static inline void create_framebuffers(re_canvas_t *canvas) {
  for (size_t i = 0; i < ARRAYSIZE(canvas->resources); i++) {
    auto &resource = canvas->resources[i];

    VkImageView attachments[]{
        resource.color.image_view,
        resource.depth.image_view,
    };

    VkFramebufferCreateInfo createInfo = {
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,     // sType
        nullptr,                                       // pNext
        0,                                             // flags
        canvas->render_target.render_pass,             // renderPass
        static_cast<uint32_t>(ARRAYSIZE(attachments)), // attachmentCount
        attachments,                                   // pAttachments
        canvas->width,                                 // width
        canvas->height,                                // height
        1,                                             // layers
    };

    VK_CHECK(vkCreateFramebuffer(
        renderer::ctx().m_device, &createInfo, nullptr, &resource.framebuffer));
  }
}

static inline void destroy_framebuffers(re_canvas_t *canvas) {
  VK_CHECK(vkDeviceWaitIdle(renderer::ctx().m_device));

  for (size_t i = 0; i < ARRAYSIZE(canvas->resources); i++) {
    auto &resource = canvas->resources[i];

    if (resource.framebuffer != VK_NULL_HANDLE) {
      vkDestroyFramebuffer(
          renderer::ctx().m_device, resource.framebuffer, nullptr);
    }
  }
}

static inline void create_render_pass(re_canvas_t *canvas) {
  VkAttachmentDescription attachmentDescriptions[] = {
      // Resolved color attachment
      VkAttachmentDescription{
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
      VkAttachmentDescription{
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
      renderer::ctx().m_device,
      &renderPassCreateInfo,
      nullptr,
      &canvas->render_target.render_pass));
}

static inline void destroy_render_pass(re_canvas_t *canvas) {
  VK_CHECK(vkDeviceWaitIdle(renderer::ctx().m_device));
  if (canvas->render_target.render_pass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(
        renderer::ctx().m_device, canvas->render_target.render_pass, nullptr);
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
  assert(renderer::ctx().getSupportedDepthFormat(&canvas->depth_format));

  create_color_target(canvas);
  create_depth_target(canvas);
  create_descriptor_sets(canvas);
  create_render_pass(canvas);
  create_framebuffers(canvas);
}

void re_canvas_begin(
    re_canvas_t *canvas, const VkCommandBuffer command_buffer) {
  auto &resource = canvas->resources[0];

  // @todo: make this customizable
  VkClearValue clearValues[2] = {};
  clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  clearValues[1].depthStencil = {1.0f, 0};

  VkRenderPassBeginInfo renderPassBeginInfo = {
      VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,  // sType
      nullptr,                                   // pNext
      canvas->render_target.render_pass,         // renderPass
      resource.framebuffer,                      // framebuffer
      {{0, 0}, {canvas->width, canvas->height}}, // renderArea
      ARRAYSIZE(clearValues),                    // clearValueCount
      clearValues,                               // pClearValues
  };

  vkCmdBeginRenderPass(
      command_buffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport{
      0.0f,                               // x
      0.0f,                               // y
      static_cast<float>(canvas->width),  // width
      static_cast<float>(canvas->height), // height
      0.0f,                               // minDepth
      1.0f,                               // maxDepth
  };

  vkCmdSetViewport(command_buffer, 0, 1, &viewport);

  VkRect2D scissor{{0, 0}, {canvas->width, canvas->height}};

  vkCmdSetScissor(command_buffer, 0, 1, &scissor);
}

void re_canvas_end(re_canvas_t *, const VkCommandBuffer command_buffer) {
  vkCmdEndRenderPass(command_buffer);
}

void re_canvas_draw(
    re_canvas_t *canvas,
    const VkCommandBuffer command_buffer,
    re_pipeline_t *pipeline) {
  auto resource = canvas->resources[0];

  vkCmdBindPipeline(
      command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);

  vkCmdBindDescriptorSets(
      command_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline->layout,
      0, // firstSet
      1,
      &resource.resource_set.descriptor_set,
      0,
      nullptr);

  vkCmdDraw(command_buffer, 3, 1, 0, 0);
}

void re_canvas_resize(
    re_canvas_t *canvas, const uint32_t width, const uint32_t height) {
  VK_CHECK(vkDeviceWaitIdle(renderer::ctx().m_device));
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
