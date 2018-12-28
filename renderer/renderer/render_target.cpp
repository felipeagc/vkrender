#include "render_target.hpp"
#include "context.hpp"
#include "pipeline.hpp"
#include "util.hpp"
#include "window.hpp"

using namespace renderer;

RenderTarget::RenderTarget(const uint32_t width, const uint32_t height)
    : m_width(width), m_height(height) {
  assert(ctx().getSupportedDepthFormat(&m_depthFormat));
  this->createColorTarget();
  this->createDepthTarget();
  this->createDescriptorSet();
  this->createRenderPass();
  this->createFramebuffers();
}

RenderTarget::~RenderTarget() {
  this->destroyFramebuffers();
  this->destroyRenderPass();
  this->destroyColorTarget();
  this->destroyDepthTarget();
  this->destroyDescriptorSet();
}

void RenderTarget::beginRenderPass(Window &window) {
  auto commandBuffer = window.getCurrentCommandBuffer();
  auto &resource =
      m_resources[window.getCurrentFrameIndex() % ARRAYSIZE(m_resources)];

  // @todo: make this customizable
  VkClearValue clearValues[2] = {};
  clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  clearValues[1].depthStencil = {1.0f, 0};

  VkRenderPassBeginInfo renderPassBeginInfo = {
      VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, // sType
      nullptr,                                  // pNext
      m_renderPass,                             // renderPass
      resource.framebuffer,                     // framebuffer
      {{0, 0}, {m_width, m_height}},            // renderArea
      ARRAYSIZE(clearValues),                   // clearValueCount
      clearValues,                              // pClearValues
  };

  vkCmdBeginRenderPass(
      commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport{
      0.0f,                         // x
      0.0f,                         // y
      static_cast<float>(m_width),  // width
      static_cast<float>(m_height), // height
      0.0f,                         // minDepth
      1.0f,                         // maxDepth
  };

  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{{0, 0}, {m_width, m_height}};

  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void RenderTarget::endRenderPass(Window &window) {
  auto commandBuffer = window.getCurrentCommandBuffer();

  vkCmdEndRenderPass(commandBuffer);
}

void RenderTarget::draw(Window &window, GraphicsPipeline &pipeline) {
  auto commandBuffer = window.getCurrentCommandBuffer();
  auto &resource =
      m_resources[window.getCurrentFrameIndex() % ARRAYSIZE(m_resources)];

  vkCmdBindPipeline(
      commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.m_pipeline);

  vkCmdBindDescriptorSets(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline.m_pipelineLayout,
      0, // firstSet
      1,
      &resource.descriptorSet,
      0,
      nullptr);

  vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

void RenderTarget::resize(const uint32_t width, const uint32_t height) {
  VK_CHECK(vkDeviceWaitIdle(ctx().m_device));
  m_width = width;
  m_height = height;
  this->destroyFramebuffers();
  this->destroyRenderPass();
  this->destroyDescriptorSet();
  this->destroyColorTarget();
  this->destroyDepthTarget();
  this->createColorTarget();
  this->createDepthTarget();
  this->createDescriptorSet();
  this->createRenderPass();
  this->createFramebuffers();
}

void RenderTarget::createColorTarget() {
  for (size_t i = 0; i < ARRAYSIZE(m_resources); i++) {
    auto &resource = m_resources[i];

    VkImageCreateInfo imageCreateInfo = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        nullptr,
        0,                // flags
        VK_IMAGE_TYPE_2D, // imageType
        m_colorFormat,    // format
        {
            m_width,             // width
            m_height,            // height
            1,                   // depth
        },                       // extent
        1,                       // mipLevels
        1,                       // arrayLayers
        m_sampleCount,           // samples
        VK_IMAGE_TILING_OPTIMAL, // tiling
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, // usage
        VK_SHARING_MODE_EXCLUSIVE,               // sharingMode
        0,                                       // queueFamilyIndexCount
        nullptr,                                 // pQueueFamilyIndices
        VK_IMAGE_LAYOUT_UNDEFINED,               // initialLayout
    };

    VmaAllocationCreateInfo imageAllocCreateInfo = {};
    imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VK_CHECK(vmaCreateImage(
        ctx().m_allocator,
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
        m_colorFormat,         // format
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
        ctx().m_device,
        &imageViewCreateInfo,
        nullptr,
        &resource.color.imageView));

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
        ctx().m_device, &samplerCreateInfo, nullptr, &resource.color.sampler));
  }
}

void RenderTarget::destroyColorTarget() {
  VK_CHECK(vkDeviceWaitIdle(ctx().m_device));
  for (size_t i = 0; i < ARRAYSIZE(m_resources); i++) {
    auto &resource = m_resources[i];

    if (resource.color.image != VK_NULL_HANDLE) {
      vkDestroyImageView(ctx().m_device, resource.color.imageView, nullptr);
    }

    if (resource.color.sampler != VK_NULL_HANDLE) {
      vkDestroySampler(ctx().m_device, resource.color.sampler, nullptr);
    }

    if (resource.color.image != VK_NULL_HANDLE) {
      vmaDestroyImage(
          ctx().m_allocator, resource.color.image, resource.color.allocation);
    }

    resource.color.image = VK_NULL_HANDLE;
    resource.color.allocation = VK_NULL_HANDLE;
    resource.color.imageView = VK_NULL_HANDLE;
    resource.color.sampler = VK_NULL_HANDLE;
  }
}

void RenderTarget::createDepthTarget() {
  for (size_t i = 0; i < ARRAYSIZE(m_resources); i++) {
    auto &resource = m_resources[i];

    VkImageCreateInfo imageCreateInfo = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, // sType
        nullptr,                             // pNext
        0,                                   // flags
        VK_IMAGE_TYPE_2D,                    // imageType
        m_depthFormat,                       // format
        {
            m_width,             // width
            m_height,            // height
            1,                   // depth
        },                       // extent
        1,                       // mipLevels
        1,                       // arrayLayers
        m_sampleCount,           // samples
        VK_IMAGE_TILING_OPTIMAL, // tiling
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
        ctx().m_allocator,
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
        m_depthFormat,                            // format
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
        ctx().m_device,
        &imageViewCreateInfo,
        nullptr,
        &resource.depth.imageView));
  }
}

void RenderTarget::destroyDepthTarget() {
  VK_CHECK(vkDeviceWaitIdle(ctx().m_device));

  for (size_t i = 0; i < ARRAYSIZE(m_resources); i++) {
    auto &resource = m_resources[i];

    if (resource.depth.image != VK_NULL_HANDLE) {
      vmaDestroyImage(
          ctx().m_allocator, resource.depth.image, resource.depth.allocation);
    }

    if (resource.depth.imageView != VK_NULL_HANDLE) {
      vkDestroyImageView(ctx().m_device, resource.depth.imageView, nullptr);
    }
  }
}

void RenderTarget::createDescriptorSet() {
  for (size_t i = 0; i < ARRAYSIZE(m_resources); i++) {
    auto &resource = m_resources[i];

    auto [descriptorPool, descriptorSetLayout] =
        ctx().m_descriptorManager[DESC_FULLSCREEN];

    assert(descriptorPool != nullptr && descriptorSetLayout != nullptr);

    VkDescriptorSetAllocateInfo allocateInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        nullptr,
        *descriptorPool,
        1,
        descriptorSetLayout,
    };

    VK_CHECK(vkAllocateDescriptorSets(
        renderer::ctx().m_device, &allocateInfo, &resource.descriptorSet));

    VkDescriptorImageInfo descriptor = {
        resource.color.sampler,
        resource.color.imageView,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet descriptorWrites[] = {
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            resource.descriptorSet,                    // dstSet
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

void RenderTarget::destroyDescriptorSet() {
  VK_CHECK(vkDeviceWaitIdle(ctx().m_device));

  for (size_t i = 0; i < ARRAYSIZE(m_resources); i++) {
    auto &resource = m_resources[i];

    VK_CHECK(vkFreeDescriptorSets(
        renderer::ctx().m_device,
        *renderer::ctx().m_descriptorManager.getPool(renderer::DESC_FULLSCREEN),
        1,
        &resource.descriptorSet));
  }
}

void RenderTarget::createFramebuffers() {
  for (size_t i = 0; i < ARRAYSIZE(m_resources); i++) {
    auto &resource = m_resources[i];

    VkImageView attachments[]{
        resource.color.imageView,
        resource.depth.imageView,
    };

    VkFramebufferCreateInfo createInfo = {
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,     // sType
        nullptr,                                       // pNext
        0,                                             // flags
        m_renderPass,                                  // renderPass
        static_cast<uint32_t>(ARRAYSIZE(attachments)), // attachmentCount
        attachments,                                   // pAttachments
        m_width,                                       // width
        m_height,                                      // height
        1,                                             // layers
    };

    VK_CHECK(vkCreateFramebuffer(
        ctx().m_device, &createInfo, nullptr, &resource.framebuffer));
  }
}

void RenderTarget::destroyFramebuffers() {
  VK_CHECK(vkDeviceWaitIdle(ctx().m_device));

  for (size_t i = 0; i < ARRAYSIZE(m_resources); i++) {
    auto &resource = m_resources[i];

    if (resource.framebuffer != VK_NULL_HANDLE) {
      vkDestroyFramebuffer(ctx().m_device, resource.framebuffer, nullptr);
    }
  }
}

void RenderTarget::createRenderPass() {
  VkAttachmentDescription attachmentDescriptions[] = {
      // Resolved color attachment
      VkAttachmentDescription{
          0,                                        // flags
          m_colorFormat,                            // format
          m_sampleCount,                            // samples
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
          m_depthFormat,                                    // format
          m_sampleCount,                                    // samples
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
      ctx().m_device, &renderPassCreateInfo, nullptr, &m_renderPass));
}

void RenderTarget::destroyRenderPass() {
  VK_CHECK(vkDeviceWaitIdle(ctx().m_device));
  if (m_renderPass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(ctx().m_device, m_renderPass, nullptr);
  }
}
