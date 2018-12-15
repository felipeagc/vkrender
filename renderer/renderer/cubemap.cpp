#include "cubemap.hpp"
#include "buffer.hpp"
#include "context.hpp"
#include "pipeline.hpp"
#include "shader.hpp"
#include "util.hpp"
#include <fstl/logging.hpp>
#include <stb_image.h>

using namespace renderer;

struct CameraUniform {
  glm::mat4 view;
  glm::mat4 proj;
};

// TODO: move this function to some utils file and use it elsewhere as well.
static void setImageLayout(
    VkCommandBuffer cmdbuffer,
    VkImage image,
    VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout,
    VkImageSubresourceRange subresourceRange,
    VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) {
  // Create an image barrier object
  VkImageMemoryBarrier imageMemoryBarrier = {};
  imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imageMemoryBarrier.pNext = nullptr;
  imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageMemoryBarrier.oldLayout = oldImageLayout;
  imageMemoryBarrier.newLayout = newImageLayout;
  imageMemoryBarrier.image = image;
  imageMemoryBarrier.subresourceRange = subresourceRange;

  // Source layouts (old)
  // Source access mask controls actions that have to be finished on the old
  // layout before it will be transitioned to the new layout
  switch (oldImageLayout) {
  case VK_IMAGE_LAYOUT_UNDEFINED:
    // Image layout is undefined (or does not matter)
    // Only valid as initial layout
    // No flags required, listed only for completeness
    imageMemoryBarrier.srcAccessMask = 0;
    break;

  case VK_IMAGE_LAYOUT_PREINITIALIZED:
    // Image is preinitialized
    // Only valid as initial layout for linear images, preserves memory contents
    // Make sure host writes have been finished
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    break;

  case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
    // Image is a color attachment
    // Make sure any writes to the color buffer have been finished
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    break;

  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    // Image is a depth/stencil attachment
    // Make sure any writes to the depth/stencil buffer have been finished
    imageMemoryBarrier.srcAccessMask =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    break;

  case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
    // Image is a transfer source
    // Make sure any reads from the image have been finished
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    break;

  case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
    // Image is a transfer destination
    // Make sure any writes to the image have been finished
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    break;

  case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
    // Image is read by a shader
    // Make sure any shader reads from the image have been finished
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    break;
  default:
    // Other source layouts aren't handled (yet)
    break;
  }

  // Target layouts (new)
  // Destination access mask controls the dependency for the new image layout
  switch (newImageLayout) {
  case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
    // Image will be used as a transfer destination
    // Make sure any writes to the image have been finished
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    break;

  case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
    // Image will be used as a transfer source
    // Make sure any reads from the image have been finished
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    break;

  case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
    // Image will be used as a color attachment
    // Make sure any writes to the color buffer have been finished
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    break;

  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    // Image layout will be used as a depth/stencil attachment
    // Make sure any writes to depth/stencil buffer have been finished
    imageMemoryBarrier.dstAccessMask =
        imageMemoryBarrier.dstAccessMask |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    break;

  case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
    // Image will be read in a shader (sampler, input attachment)
    // Make sure any writes to the image have been finished
    if (imageMemoryBarrier.srcAccessMask == 0) {
      imageMemoryBarrier.srcAccessMask =
          VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    break;
  default:
    // Other source layouts aren't handled (yet)
    break;
  }

  // Put barrier inside setup command buffer
  vkCmdPipelineBarrier(
      cmdbuffer,
      srcStageMask,
      dstStageMask,
      0,
      0,
      nullptr,
      0,
      nullptr,
      1,
      &imageMemoryBarrier);
}

static void setImageLayout(
    VkCommandBuffer cmdbuffer,
    VkImage image,
    VkImageAspectFlags aspectMask,
    VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout,
    VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) {
  VkImageSubresourceRange subresourceRange = {};
  subresourceRange.aspectMask = aspectMask;
  subresourceRange.baseMipLevel = 0;
  subresourceRange.levelCount = 1;
  subresourceRange.layerCount = 1;

  setImageLayout(
      cmdbuffer,
      image,
      oldImageLayout,
      newImageLayout,
      subresourceRange,
      srcStageMask,
      dstStageMask);
}

static void bakeCubemap(
    const std::string &hdrFile,
    VkImage &cubemapImage,
    VkFormat cubemapImageFormat,
    uint32_t cubemapImageWidth,
    uint32_t cubemapImageHeight) {
  // Load HDR image
  stbi_set_flip_vertically_on_load(true);
  int hdrWidth, hdrHeight, nrComponents;
  float *hdrData =
      stbi_loadf(hdrFile.c_str(), &hdrWidth, &hdrHeight, &nrComponents, 4);
  stbi_set_flip_vertically_on_load(false);

  // Create HDR VkImage and stuff
  VkImage hdrImage = VK_NULL_HANDLE;
  VmaAllocation hdrAllocation = VK_NULL_HANDLE;
  VkImageView hdrImageView = VK_NULL_HANDLE;
  VkSampler hdrSampler = VK_NULL_HANDLE;

  {
    VkImageCreateInfo imageCreateInfo = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        nullptr,
        0,                  // flags
        VK_IMAGE_TYPE_2D,   // imageType
        cubemapImageFormat, // format
        {
            static_cast<uint32_t>(hdrWidth),  // width
            static_cast<uint32_t>(hdrHeight), // height
            1,                                // depth
        },                                    // extent
        1,                                    // mipLevels
        1,                                    // arrayLayers
        VK_SAMPLE_COUNT_1_BIT,                // samples
        VK_IMAGE_TILING_OPTIMAL,              // tiling
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, // usage
        VK_SHARING_MODE_EXCLUSIVE, // sharingMode
        0,                         // queueFamilyIndexCount
        nullptr,                   // pQueueFamilyIndices
        VK_IMAGE_LAYOUT_UNDEFINED, // initialLayout
    };

    VmaAllocationCreateInfo imageAllocCreateInfo = {};
    imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VK_CHECK(vmaCreateImage(
        renderer::ctx().m_allocator,
        &imageCreateInfo,
        &imageAllocCreateInfo,
        &hdrImage,
        &hdrAllocation,
        nullptr));

    VkImageViewCreateInfo imageViewCreateInfo = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr,
        0,                     // flags
        hdrImage,              // image
        VK_IMAGE_VIEW_TYPE_2D, // viewType
        cubemapImageFormat,    // format
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
        &hdrImageView));

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
        renderer::ctx().m_device, &samplerCreateInfo, nullptr, &hdrSampler));
  }

  // Create side VkImage and stuff
  VkImage sideImage = VK_NULL_HANDLE;
  VmaAllocation sideAllocation = VK_NULL_HANDLE;
  VkImageView sideImageView = VK_NULL_HANDLE;

  {
    VkImageCreateInfo imageCreateInfo = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        nullptr,
        0,                  // flags
        VK_IMAGE_TYPE_2D,   // imageType
        cubemapImageFormat, // format
        {
            cubemapImageWidth,   // width
            cubemapImageHeight,  // height
            1,                   // depth
        },                       // extent
        1,                       // mipLevels
        1,                       // arrayLayers
        VK_SAMPLE_COUNT_1_BIT,   // samples
        VK_IMAGE_TILING_OPTIMAL, // tiling
        VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT, // usage
        VK_SHARING_MODE_EXCLUSIVE,           // sharingMode
        0,                                   // queueFamilyIndexCount
        nullptr,                             // pQueueFamilyIndices
        VK_IMAGE_LAYOUT_UNDEFINED,           // initialLayout
    };

    VmaAllocationCreateInfo imageAllocCreateInfo = {};
    imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VK_CHECK(vmaCreateImage(
        renderer::ctx().m_allocator,
        &imageCreateInfo,
        &imageAllocCreateInfo,
        &sideImage,
        &sideAllocation,
        nullptr));

    VkImageViewCreateInfo imageViewCreateInfo = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr,
        0,                     // flags
        sideImage,             // image
        VK_IMAGE_VIEW_TYPE_2D, // viewType
        cubemapImageFormat,    // format
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
        &sideImageView));
  }

  // Upload data to image
  size_t hdrSize = hdrWidth * hdrHeight * 4 * sizeof(float);
  renderer::Buffer stagingBuffer{renderer::BufferType::eStaging, hdrSize};

  void *stagingMemoryPointer;
  stagingBuffer.mapMemory(&stagingMemoryPointer);
  memcpy(stagingMemoryPointer, hdrData, hdrSize);

  stagingBuffer.imageTransfer(
      hdrImage,
      static_cast<uint32_t>(hdrWidth),
      static_cast<uint32_t>(hdrHeight));

  stagingBuffer.unmapMemory();
  stagingBuffer.destroy();

  // Create hdrDescriptorSet
  VkDescriptorSet hdrDescriptorSet;

  auto [hdrDescriptorPool, hdrDescriptorSetLayout] =
      renderer::ctx().m_descriptorManager[renderer::DESC_MATERIAL];

  assert(hdrDescriptorPool != nullptr && hdrDescriptorSetLayout != nullptr);

  VkDescriptorSetAllocateInfo hdrDescriptorSetAllocateInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      nullptr,
      *hdrDescriptorPool,
      1,
      hdrDescriptorSetLayout,
  };

  VK_CHECK(vkAllocateDescriptorSets(
      renderer::ctx().m_device,
      &hdrDescriptorSetAllocateInfo,
      &hdrDescriptorSet));

  VkDescriptorImageInfo hdrImageDescriptor = {
      hdrSampler,
      hdrImageView,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };

  VkWriteDescriptorSet hdrDescriptorWrite = {
      VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      nullptr,
      hdrDescriptorSet,                          // dstSet
      1,                                         // dstBinding
      0,                                         // dstArrayElement
      1,                                         // descriptorCount
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
      &hdrImageDescriptor,                       // pImageInfo
      nullptr,                                   // pBufferInfo
      nullptr,                                   // pTexelBufferView
  };

  vkUpdateDescriptorSets(
      renderer::ctx().m_device, 1, &hdrDescriptorWrite, 0, nullptr);

  // Camera matrices
  CameraUniform cameraUBO;

  cameraUBO.proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

  glm::mat4 cameraViews[] = {
      glm::lookAt(
          glm::vec3(0.0f, 0.0f, 0.0f),
          glm::vec3(1.0f, 0.0f, 0.0f),
          glm::vec3(0.0f, -1.0f, 0.0f)),
      glm::lookAt(
          glm::vec3(0.0f, 0.0f, 0.0f),
          glm::vec3(-1.0f, 0.0f, 0.0f),
          glm::vec3(0.0f, -1.0f, 0.0f)),
      glm::lookAt(
          glm::vec3(0.0f, 0.0f, 0.0f),
          glm::vec3(0.0f, 1.0f, 0.0f),
          glm::vec3(0.0f, 0.0f, 1.0f)),
      glm::lookAt(
          glm::vec3(0.0f, 0.0f, 0.0f),
          glm::vec3(0.0f, -1.0f, 0.0f),
          glm::vec3(0.0f, 0.0f, -1.0f)),
      glm::lookAt(
          glm::vec3(0.0f, 0.0f, 0.0f),
          glm::vec3(0.0f, 0.0f, 1.0f),
          glm::vec3(0.0f, -1.0f, 0.0f)),
      glm::lookAt(
          glm::vec3(0.0f, 0.0f, 0.0f),
          glm::vec3(0.0f, 0.0f, -1.0f),
          glm::vec3(0.0f, -1.0f, 0.0f)),
  };

  // TODO: use push constants instead
  // Create cameraDescriptorSet
  void *cameraUniformMappings[6];
  VkDescriptorSet cameraDescriptorSets[6] = {};
  renderer::Buffer cameraUniformBuffers[6] = {};

  auto [cameraDescriptorPool, cameraDescriptorSetLayout] =
      renderer::ctx().m_descriptorManager[renderer::DESC_CAMERA];

  assert(
      cameraDescriptorPool != nullptr && cameraDescriptorSetLayout != nullptr);

  for (size_t i = 0; i < 6; i++) {
    cameraUniformBuffers[i] =
        renderer::Buffer{renderer::BufferType::eUniform, sizeof(CameraUniform)};

    cameraUniformBuffers[i].mapMemory(&cameraUniformMappings[i]);

    cameraUBO.view = cameraViews[i];

    memcpy(cameraUniformMappings[i], &cameraUBO, sizeof(CameraUniform));

    VkDescriptorSetAllocateInfo cameraDescriptorAllocateInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        nullptr,
        *cameraDescriptorPool,
        1,
        cameraDescriptorSetLayout,
    };

    vkAllocateDescriptorSets(
        renderer::ctx().m_device,
        &cameraDescriptorAllocateInfo,
        &cameraDescriptorSets[i]);

    VkDescriptorBufferInfo bufferInfo{
        cameraUniformBuffers[i].getHandle(),
        0,
        sizeof(CameraUniform),
    };

    VkWriteDescriptorSet cameraDescriptorWrite = {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        nullptr,
        cameraDescriptorSets[i],           // dstSet
        0,                                 // dstBinding
        0,                                 // dstArrayElement
        1,                                 // descriptorCount
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
        nullptr,                           // pImageInfo
        &bufferInfo,                       // pBufferInfo
        nullptr,                           // pTexelBufferView
    };

    vkUpdateDescriptorSets(
        renderer::ctx().m_device, 1, &cameraDescriptorWrite, 0, nullptr);
  }

  // Create Fence
  VkFence fence = VK_NULL_HANDLE;

  VkFenceCreateInfo fenceCreateInfo = {};
  fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceCreateInfo.flags = 0;

  VK_CHECK(vkCreateFence(
      renderer::ctx().m_device, &fenceCreateInfo, nullptr, &fence));

  // Create renderpass
  VkRenderPass renderPass = VK_NULL_HANDLE;

  VkAttachmentDescription attachment = {};
  attachment.format = cubemapImageFormat;
  attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference colorAttachment = {};
  colorAttachment.attachment = 0;
  colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachment;

  VkSubpassDependency dependencies[2] = {};
  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[1].srcSubpass = 0;
  dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  VkRenderPassCreateInfo renderPassCreateInfo = {
      VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      nullptr,                 // pNext
      0,                       // flags
      1,                       // attachmentCount
      &attachment,             // pAttachments
      1,                       // subpassCount
      &subpass,                // pSubpasses
      ARRAYSIZE(dependencies), // dependencyCount
      dependencies,            // pDependencies
  };

  VK_CHECK(vkCreateRenderPass(
      renderer::ctx().m_device, &renderPassCreateInfo, nullptr, &renderPass));

  // Create framebuffer
  VkFramebuffer framebuffer = VK_NULL_HANDLE;

  VkFramebufferCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, // sType
      nullptr,                                   // pNext
      0,                                         // flags
      renderPass,                                // renderPass
      1,                                         // attachmentCount
      &sideImageView,                            // pAttachments
      cubemapImageWidth,                         // width
      cubemapImageHeight,                        // height
      1,                                         // layers
  };

  VK_CHECK(vkCreateFramebuffer(
      renderer::ctx().m_device, &createInfo, nullptr, &framebuffer));

  // Create pipeline
  renderer::Shader shader(
      "../shaders/bake_cubemap.vert", "../shaders/bake_cubemap.frag");
  renderer::BakeCubemapPipeline pipeline(renderPass, shader);
  shader.destroy();

  // Allocate command buffer
  VkCommandBufferAllocateInfo allocateInfo = {};
  allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocateInfo.pNext = nullptr;
  allocateInfo.commandPool = renderer::ctx().m_graphicsCommandPool;
  allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocateInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

  vkAllocateCommandBuffers(
      renderer::ctx().m_device, &allocateInfo, &commandBuffer);

  // Begin command buffer
  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  beginInfo.pInheritanceInfo = nullptr;

  VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

  // Begin renderpass
  VkClearValue clearValue = {{{1.0f, 1.0f, 1.0f, 1.0f}}};

  VkRenderPassBeginInfo renderPassBeginInfo = {
      VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,          // sType
      nullptr,                                           // pNext
      renderPass,                                        // renderPass
      framebuffer,                                       // framebuffer
      {{0, 0}, {cubemapImageWidth, cubemapImageHeight}}, // renderArea
      1,                                                 // clearValueCount
      &clearValue,                                       // pClearValues
  };

  {
    VkImageSubresourceRange cubeFaceSubresourceRange = {};
    cubeFaceSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    cubeFaceSubresourceRange.baseMipLevel = 0;
    cubeFaceSubresourceRange.levelCount = 1;
    cubeFaceSubresourceRange.baseArrayLayer = 0;
    cubeFaceSubresourceRange.layerCount = 6;

    setImageLayout(
        commandBuffer,
        cubemapImage,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        cubeFaceSubresourceRange);
  }

  for (size_t i = 0; i < ARRAYSIZE(cameraViews); i++) {
    vkCmdBeginRenderPass(
        commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{
        0.0f,                                   // x
        0.0f,                                   // y
        static_cast<float>(cubemapImageWidth),  // width
        static_cast<float>(cubemapImageHeight), // height
        0.0f,                                   // minDepth
        1.0f,                                   // maxDepth
    };

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{{0, 0}, {cubemapImageWidth, cubemapImageHeight}};

    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // Do stuff
    {
      vkCmdBindPipeline(
          commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.m_pipeline);

      vkCmdBindDescriptorSets(
          commandBuffer,
          VK_PIPELINE_BIND_POINT_GRAPHICS,
          pipeline.m_pipelineLayout,
          1, // firstSet
          1,
          &hdrDescriptorSet,
          0,
          nullptr);

      vkCmdBindDescriptorSets(
          commandBuffer,
          VK_PIPELINE_BIND_POINT_GRAPHICS,
          pipeline.m_pipelineLayout,
          0, // firstSet
          1,
          &cameraDescriptorSets[i],
          0,
          nullptr);

      vkCmdDraw(commandBuffer, 36, 1, 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);

    setImageLayout(
        commandBuffer,
        sideImage,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    VkImageSubresourceRange cubeFaceSubresourceRange = {};
    cubeFaceSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    cubeFaceSubresourceRange.baseMipLevel = 0;
    cubeFaceSubresourceRange.levelCount = 1;
    cubeFaceSubresourceRange.baseArrayLayer = i;
    cubeFaceSubresourceRange.layerCount = 1;

    setImageLayout(
        commandBuffer,
        cubemapImage,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        cubeFaceSubresourceRange);

    VkImageCopy copyRegion = {};

    copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.srcSubresource.baseArrayLayer = 0;
    copyRegion.srcSubresource.mipLevel = 0;
    copyRegion.srcSubresource.layerCount = 1;
    copyRegion.srcOffset = {0, 0, 0};

    copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.dstSubresource.baseArrayLayer = i;
    copyRegion.dstSubresource.mipLevel = 0;
    copyRegion.dstSubresource.layerCount = 1;
    copyRegion.dstOffset = {0, 0, 0};

    copyRegion.extent.width = cubemapImageWidth;
    copyRegion.extent.height = cubemapImageHeight;
    copyRegion.extent.depth = 1;

    // Put image copy into command buffer
    vkCmdCopyImage(
        commandBuffer,
        sideImage,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        cubemapImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &copyRegion);

    // Transform framebuffer color attachment back
    setImageLayout(
        commandBuffer,
        sideImage,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    // Change image layout of copied face to shader read
    setImageLayout(
        commandBuffer,
        cubemapImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        cubeFaceSubresourceRange);
  }

  VK_CHECK(vkEndCommandBuffer(commandBuffer));

  // Submit
  VkPipelineStageFlags waitDstStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  VkSubmitInfo submitInfo = {
      VK_STRUCTURE_TYPE_SUBMIT_INFO, // sType
      nullptr,                       // pNext
      0,                             // waitSemaphoreCount
      nullptr,                       // pWaitSemaphores
      &waitDstStageMask,             // pWaitDstStageMask
      1,                             // commandBufferCount
      &commandBuffer,                // pCommandBuffers
      0,                             // signalSemaphoreCount
      nullptr,                       // pSignalSemaphores
  };

  vkQueueSubmit(renderer::ctx().m_graphicsQueue, 1, &submitInfo, fence);

  VK_CHECK(vkWaitForFences(
      renderer::ctx().m_device, 1, &fence, VK_TRUE, UINT64_MAX));

  VK_CHECK(vkDeviceWaitIdle(renderer::ctx().m_device));

  vkDestroyImageView(renderer::ctx().m_device, sideImageView, nullptr);
  vmaDestroyImage(renderer::ctx().m_allocator, sideImage, sideAllocation);

  vkDestroyImageView(renderer::ctx().m_device, hdrImageView, nullptr);
  vkDestroySampler(renderer::ctx().m_device, hdrSampler, nullptr);
  vmaDestroyImage(renderer::ctx().m_allocator, hdrImage, hdrAllocation);

  vkFreeDescriptorSets(
      renderer::ctx().m_device,
      *cameraDescriptorPool,
      ARRAYSIZE(cameraDescriptorSets),
      cameraDescriptorSets);

  vkFreeDescriptorSets(
      renderer::ctx().m_device, *hdrDescriptorPool, 1, &hdrDescriptorSet);

  for (size_t i = 0; i < ARRAYSIZE(cameraUniformBuffers); i++) {
    cameraUniformBuffers[i].destroy();
  }

  vkDestroyFence(renderer::ctx().m_device, fence, nullptr);

  vkFreeCommandBuffers(
      renderer::ctx().m_device,
      renderer::ctx().m_graphicsCommandPool,
      1,
      &commandBuffer);

  vkDestroyFramebuffer(renderer::ctx().m_device, framebuffer, nullptr);

  vkDestroyRenderPass(renderer::ctx().m_device, renderPass, nullptr);

  stbi_image_free(hdrData);
}

static void createCubemapImage(
    VkImage *image,
    VmaAllocation *allocation,
    VkImageView *imageView,
    VkSampler *sampler,
    VkFormat format,
    uint32_t width,
    uint32_t height) {
  VkImageCreateInfo imageCreateInfo = {
      VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      nullptr,                             // pNext
      VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, // flags
      VK_IMAGE_TYPE_2D,                    // imageType
      format,                              // format
      {
          width,               // width
          height,              // height
          1,                   // depth
      },                       // extent
      1,                       // mipLevels
      6,                       // arrayLayers
      VK_SAMPLE_COUNT_1_BIT,   // samples
      VK_IMAGE_TILING_OPTIMAL, // tiling
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, // usage
      VK_SHARING_MODE_EXCLUSIVE, // sharingMode
      0,                         // queueFamilyIndexCount
      nullptr,                   // pQueueFamilyIndices
      VK_IMAGE_LAYOUT_UNDEFINED, // initialLayout
  };

  VmaAllocationCreateInfo imageAllocCreateInfo = {};
  imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  VK_CHECK(vmaCreateImage(
      ctx().m_allocator,
      &imageCreateInfo,
      &imageAllocCreateInfo,
      image,
      allocation,
      nullptr));

  VkImageViewCreateInfo imageViewCreateInfo = {
      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      nullptr,
      0,                       // flags
      *image,                  // image
      VK_IMAGE_VIEW_TYPE_CUBE, // viewType
      format,                  // format
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
          6,                         // layerCount
      },                             // subresourceRange
  };

  VK_CHECK(vkCreateImageView(
      ctx().m_device, &imageViewCreateInfo, nullptr, imageView));

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

  VK_CHECK(
      vkCreateSampler(ctx().m_device, &samplerCreateInfo, nullptr, sampler));
}

Cubemap::Cubemap(
    const std::string &hdrPath, const uint32_t width, const uint32_t height) {
  m_width = width;
  m_height = height;

  createCubemapImage(
      &m_image,
      &m_allocation,
      &m_imageView,
      &m_sampler,
      VK_FORMAT_R32G32B32A32_SFLOAT,
      width,
      height);

  bakeCubemap(hdrPath, m_image, VK_FORMAT_R32G32B32A32_SFLOAT, width, height);
}

Cubemap::operator bool() const { return m_image != VK_NULL_HANDLE; }

VkDescriptorImageInfo Cubemap::getDescriptorInfo() const {
  return {
      m_sampler,
      m_imageView,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };
}

void Cubemap::destroy() {
  VK_CHECK(vkDeviceWaitIdle(ctx().m_device));
  vkDestroyImageView(ctx().m_device, m_imageView, nullptr);
  vkDestroySampler(ctx().m_device, m_sampler, nullptr);
  vmaDestroyImage(ctx().m_allocator, m_image, m_allocation);

  m_image = VK_NULL_HANDLE;
  m_allocation = VK_NULL_HANDLE;
  m_imageView = VK_NULL_HANDLE;
  m_sampler = VK_NULL_HANDLE;
}
