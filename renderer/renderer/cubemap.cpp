#include "cubemap.hpp"
#include "buffer.hpp"
#include "canvas.hpp"
#include "context.hpp"
#include "pipeline.hpp"
#include "shader.hpp"
#include "util.hpp"
#include <fstd/array.h>
#include <fstd/file.h>
#include <fstd/task_scheduler.h>
#include <stb_image.h>
#include <string.h>

struct camera_uniform_t {
  mat4_t view;
  mat4_t proj;
};

static const mat4_t camera_views[] = {
    mat4_look_at(
        vec3_t{0.0f, 0.0f, 0.0f},
        vec3_t{1.0f, 0.0f, 0.0f},
        vec3_t{0.0f, -1.0f, 0.0f}),
    mat4_look_at(
        vec3_t{0.0f, 0.0f, 0.0f},
        vec3_t{-1.0f, 0.0f, 0.0f},
        vec3_t{0.0f, -1.0f, 0.0f}),
    mat4_look_at(
        vec3_t{0.0f, 0.0f, 0.0f},
        vec3_t{0.0f, 1.0f, 0.0f},
        vec3_t{0.0f, 0.0f, 1.0f}),
    mat4_look_at(
        vec3_t{0.0f, 0.0f, 0.0f},
        vec3_t{0.0f, -1.0f, 0.0f},
        vec3_t{0.0f, 0.0f, -1.0f}),
    mat4_look_at(
        vec3_t{0.0f, 0.0f, 0.0f},
        vec3_t{0.0f, 0.0f, 1.0f},
        vec3_t{0.0f, -1.0f, 0.0f}),
    mat4_look_at(
        vec3_t{0.0f, 0.0f, 0.0f},
        vec3_t{0.0f, 0.0f, -1.0f},
        vec3_t{0.0f, -1.0f, 0.0f}),
};

static void create_image_and_image_view(
    VkImage *image,
    VmaAllocation *allocation,
    VkImageView *imageView,
    VkFormat format,
    uint32_t width,
    uint32_t height,
    VkImageUsageFlags usage) {
  VkImageCreateInfo imageCreateInfo = {
      VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      NULL,
      0,                // flags
      VK_IMAGE_TYPE_2D, // imageType
      format,           // format
      {
          width,                 // width
          height,                // height
          1,                     // depth
      },                         // extent
      1,                         // mipLevels
      1,                         // arrayLayers
      VK_SAMPLE_COUNT_1_BIT,     // samples
      VK_IMAGE_TILING_OPTIMAL,   // tiling
      usage,                     // usage
      VK_SHARING_MODE_EXCLUSIVE, // sharingMode
      0,                         // queueFamilyIndexCount
      NULL,                      // pQueueFamilyIndices
      VK_IMAGE_LAYOUT_UNDEFINED, // initialLayout
  };

  VmaAllocationCreateInfo imageAllocCreateInfo = {};
  imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  VK_CHECK(vmaCreateImage(
      g_ctx.gpu_allocator,
      &imageCreateInfo,
      &imageAllocCreateInfo,
      image,
      allocation,
      NULL));

  VkImageViewCreateInfo imageViewCreateInfo = {
      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      NULL,
      0,                     // flags
      *image,                // image
      VK_IMAGE_VIEW_TYPE_2D, // viewType
      format,                // format
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

  VK_CHECK(
      vkCreateImageView(g_ctx.device, &imageViewCreateInfo, NULL, imageView));
}

static void create_sampler(VkSampler *sampler) {
  VkSamplerCreateInfo samplerCreateInfo = {
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

  VK_CHECK(vkCreateSampler(g_ctx.device, &samplerCreateInfo, NULL, sampler));
}

static void copy_side_image_to_cubemap(
    VkCommandBuffer commandBuffer,
    VkImage sideImage,
    VkImage cubemapImage,
    uint32_t cubemapWidth,
    uint32_t cubemapHeight,
    uint32_t layer,
    uint32_t level) {
  re_set_image_layout(
      commandBuffer,
      sideImage,
      VK_IMAGE_ASPECT_COLOR_BIT,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

  VkImageSubresourceRange cubeFaceSubresourceRange = {};
  cubeFaceSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  cubeFaceSubresourceRange.baseMipLevel = level;
  cubeFaceSubresourceRange.levelCount = 1;
  cubeFaceSubresourceRange.baseArrayLayer = layer;
  cubeFaceSubresourceRange.layerCount = 1;

  re_set_image_layout(
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
  copyRegion.dstSubresource.baseArrayLayer = layer;
  copyRegion.dstSubresource.mipLevel = level;
  copyRegion.dstSubresource.layerCount = 1;
  copyRegion.dstOffset = {0, 0, 0};

  copyRegion.extent.width = cubemapWidth;
  copyRegion.extent.height = cubemapHeight;
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
  re_set_image_layout(
      commandBuffer,
      sideImage,
      VK_IMAGE_ASPECT_COLOR_BIT,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  // Change image layout of copied face to shader read
  re_set_image_layout(
      commandBuffer,
      cubemapImage,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      cubeFaceSubresourceRange);
}

static void bake_cubemap(
    const char *hdrFile,
    VkImage &cubemapImage,
    VkFormat cubemapImageFormat,
    uint32_t cubemapImageWidth,
    uint32_t cubemapImageHeight,
    uint32_t level = 0) {
  // Load HDR image
  int hdrWidth, hdrHeight, nrComponents;
  float *hdrData = stbi_loadf(hdrFile, &hdrWidth, &hdrHeight, &nrComponents, 4);

  assert(hdrData != NULL);

  // Create HDR VkImage and stuff
  VkImage hdrImage = VK_NULL_HANDLE;
  VmaAllocation hdrAllocation = VK_NULL_HANDLE;
  VkImageView hdrImageView = VK_NULL_HANDLE;
  VkSampler hdrSampler = VK_NULL_HANDLE;

  create_image_and_image_view(
      &hdrImage,
      &hdrAllocation,
      &hdrImageView,
      cubemapImageFormat,
      (uint32_t)hdrWidth,
      (uint32_t)hdrHeight,
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

  create_sampler(&hdrSampler);

  // Upload data to image
  {
    size_t hdrSize = hdrWidth * hdrHeight * 4 * sizeof(float);
    re_buffer_t staging_buffer;
    re_buffer_init_staging(&staging_buffer, hdrSize);

    void *stagingMemoryPointer;
    re_buffer_map_memory(&staging_buffer, &stagingMemoryPointer);
    memcpy(stagingMemoryPointer, hdrData, hdrSize);

    re_buffer_transfer_to_image(
        &staging_buffer, hdrImage, (uint32_t)hdrWidth, (uint32_t)hdrHeight);

    re_buffer_unmap_memory(&staging_buffer);

    re_buffer_destroy(&staging_buffer);
  }

  // Create hdrDescriptorSet
  VkDescriptorSet hdr_descriptor_set;
  {
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.descriptorPool = g_ctx.descriptor_pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &g_ctx.bake_cubemap_descriptor_set_layout;

    VK_CHECK(vkAllocateDescriptorSets(
        g_ctx.device, &alloc_info, &hdr_descriptor_set));
  }

  {
    VkDescriptorImageInfo hdrImageDescriptor = {
        hdrSampler,
        hdrImageView,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet hdrDescriptorWrite = {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        NULL,
        hdr_descriptor_set,                        // dstSet
        1,                                         // dstBinding
        0,                                         // dstArrayElement
        1,                                         // descriptorCount
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
        &hdrImageDescriptor,                       // pImageInfo
        NULL,                                      // pBufferInfo
        NULL,                                      // pTexelBufferView
    };

    vkUpdateDescriptorSets(g_ctx.device, 1, &hdrDescriptorWrite, 0, NULL);
  }

  // Camera matrices
  camera_uniform_t cameraUBO;
  cameraUBO.proj = mat4_perspective(to_radians(90.0f), 1.0f, 0.1f, 10.0f);

  re_canvas_t canvas;
  re_canvas_init(
      &canvas, cubemapImageWidth, cubemapImageHeight, cubemapImageFormat);

  // Create pipeline
  re_pipeline_parameters_t pipeline_params = re_default_pipeline_parameters();
  pipeline_params.vertex_input_state = VkPipelineVertexInputStateCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, // sType
      NULL,                                                      // pNext
      0,                                                         // flags
      0,    // vertexBindingDescriptionCount
      NULL, // pVertexBindingDescriptions
      0,    // vertexAttributeDescriptionCount
      NULL, // pVertexAttributeDescriptions
  };

  VkDescriptorSetLayout set_layouts[] = {
      g_ctx.bake_cubemap_descriptor_set_layout,
  };

  VkPushConstantRange push_constant_range = {};
  push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  push_constant_range.offset = 0;
  push_constant_range.size = 128;

  VkPipelineLayoutCreateInfo create_info;
  create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.setLayoutCount = ARRAYSIZE(set_layouts);
  create_info.pSetLayouts = set_layouts;
  create_info.pushConstantRangeCount = 1;
  create_info.pPushConstantRanges = &push_constant_range;

  VK_CHECK(vkCreatePipelineLayout(
      g_ctx.device, &create_info, NULL, &pipeline_params.pipeline_layout));

  re_pipeline_t pipeline;

  const char *vertex_path = "../shaders/bake_cubemap.vert";
  const char *fragment_path = "../shaders/bake_cubemap.frag";

  re_shader_t shader;
  char *vertex_code = fstd_load_string_from_file(vertex_path);
  char *fragment_code = fstd_load_string_from_file(fragment_path);

  re_shader_init_glsl(
      &shader, vertex_path, vertex_code, fragment_path, fragment_code);

  re_pipeline_init_graphics(
      &pipeline, canvas.render_target, &shader, pipeline_params);

  free(vertex_code);
  free(fragment_code);

  // Allocate command buffer
  assert(fstd_worker_id < RE_THREAD_COUNT);
  VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
  {
    VkCommandBufferAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.pNext = NULL;
    allocateInfo.commandPool = g_ctx.thread_command_pools[fstd_worker_id];
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;

    vkAllocateCommandBuffers(g_ctx.device, &allocateInfo, &commandBuffer);
  }

  // Begin command buffer
  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  beginInfo.pInheritanceInfo = NULL;

  VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

  // Convert all layers of cubemapImage to shader read only
  {
    VkImageSubresourceRange cubeFaceSubresourceRange = {};
    cubeFaceSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    cubeFaceSubresourceRange.baseMipLevel = level;
    cubeFaceSubresourceRange.levelCount = 1;
    cubeFaceSubresourceRange.baseArrayLayer = 0;
    cubeFaceSubresourceRange.layerCount = 6; // all layers

    re_set_image_layout(
        commandBuffer,
        cubemapImage,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        cubeFaceSubresourceRange);
  }

  for (size_t i = 0; i < ARRAYSIZE(camera_views); i++) {
    re_canvas_begin(&canvas, commandBuffer);

    cameraUBO.view = camera_views[i];

    vkCmdPushConstants(
        commandBuffer,
        pipeline.layout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(camera_uniform_t),
        &cameraUBO);

    VkViewport viewport{
        0.0f,                      // x
        0.0f,                      // y
        (float)cubemapImageWidth,  // width
        (float)cubemapImageHeight, // height
        0.0f,                      // minDepth
        1.0f,                      // maxDepth
    };

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{{0, 0}, {cubemapImageWidth, cubemapImageHeight}};

    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // Do stuff
    {
      vkCmdBindPipeline(
          commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

      vkCmdBindDescriptorSets(
          commandBuffer,
          VK_PIPELINE_BIND_POINT_GRAPHICS,
          pipeline.layout,
          0, // firstSet
          1,
          &hdr_descriptor_set,
          0,
          NULL);

      vkCmdDraw(commandBuffer, 36, 1, 0, 0);
    }

    re_canvas_end(&canvas, commandBuffer);

    copy_side_image_to_cubemap(
        commandBuffer,
        canvas.resources[0].color.image,
        cubemapImage,
        cubemapImageWidth,
        cubemapImageHeight,
        i,
        level);
  }

  VK_CHECK(vkEndCommandBuffer(commandBuffer));

  // Submit
  VkPipelineStageFlags waitDstStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  VkSubmitInfo submitInfo = {
      VK_STRUCTURE_TYPE_SUBMIT_INFO, // sType
      NULL,                          // pNext
      0,                             // waitSemaphoreCount
      NULL,                          // pWaitSemaphores
      &waitDstStageMask,             // pWaitDstStageMask
      1,                             // commandBufferCount
      &commandBuffer,                // pCommandBuffers
      0,                             // signalSemaphoreCount
      NULL,                          // pSignalSemaphores
  };

  fstd_mutex_lock(&g_ctx.queue_mutex);
  vkQueueSubmit(g_ctx.graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
  fstd_mutex_unlock(&g_ctx.queue_mutex);

  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  vkDestroyImageView(g_ctx.device, hdrImageView, NULL);
  vkDestroySampler(g_ctx.device, hdrSampler, NULL);
  vmaDestroyImage(g_ctx.gpu_allocator, hdrImage, hdrAllocation);

  assert(fstd_worker_id < RE_THREAD_COUNT);
  vkFreeCommandBuffers(
      g_ctx.device,
      g_ctx.thread_command_pools[fstd_worker_id],
      1,
      &commandBuffer);

  vkFreeDescriptorSets(
      g_ctx.device, g_ctx.descriptor_pool, 1, &hdr_descriptor_set);

  stbi_image_free(hdrData);

  re_canvas_destroy(&canvas);

  re_shader_destroy(&shader);
  re_pipeline_destroy(&pipeline);
  vkDestroyPipelineLayout(g_ctx.device, pipeline_params.pipeline_layout, NULL);
}

static void create_cubemap_image(
    VkImage *image,
    VmaAllocation *allocation,
    VkImageView *imageView,
    VkSampler *sampler,
    VkFormat format,
    uint32_t width,
    uint32_t height,
    uint32_t levels = 1) {
  VkImageCreateInfo imageCreateInfo = {
      VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      NULL,                                // pNext
      VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, // flags
      VK_IMAGE_TYPE_2D,                    // imageType
      format,                              // format
      {
          width,               // width
          height,              // height
          1,                   // depth
      },                       // extent
      levels,                  // mipLevels
      6,                       // arrayLayers
      VK_SAMPLE_COUNT_1_BIT,   // samples
      VK_IMAGE_TILING_OPTIMAL, // tiling
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, // usage
      VK_SHARING_MODE_EXCLUSIVE, // sharingMode
      0,                         // queueFamilyIndexCount
      NULL,                      // pQueueFamilyIndices
      VK_IMAGE_LAYOUT_UNDEFINED, // initialLayout
  };

  VmaAllocationCreateInfo imageAllocCreateInfo = {};
  imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  VK_CHECK(vmaCreateImage(
      g_ctx.gpu_allocator,
      &imageCreateInfo,
      &imageAllocCreateInfo,
      image,
      allocation,
      NULL));

  VkImageViewCreateInfo imageViewCreateInfo = {
      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      NULL,
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
          levels,                    // levelCount
          0,                         // baseArrayLayer
          6,                         // layerCount
      },                             // subresourceRange
  };

  VK_CHECK(
      vkCreateImageView(g_ctx.device, &imageViewCreateInfo, NULL, imageView));

  VkSamplerCreateInfo samplerCreateInfo = {
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
      (float)levels,                           // maxLod
      VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK, // borderColor
      VK_FALSE,                                // unnormalizedCoordinates
  };

  VK_CHECK(vkCreateSampler(g_ctx.device, &samplerCreateInfo, NULL, sampler));
}

void re_cubemap_init_from_hdr_equirec(
    re_cubemap_t *cubemap,
    const char *path,
    const uint32_t width,
    const uint32_t height) {
  cubemap->width = width;
  cubemap->height = height;

  create_cubemap_image(
      &cubemap->image,
      &cubemap->allocation,
      &cubemap->image_view,
      &cubemap->sampler,
      VK_FORMAT_R32G32B32A32_SFLOAT,
      width,
      height);

  bake_cubemap(
      path, cubemap->image, VK_FORMAT_R32G32B32A32_SFLOAT, width, height);

  cubemap->descriptor = VkDescriptorImageInfo{
      cubemap->sampler,
      cubemap->image_view,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };
}

void re_cubemap_init_from_hdr_equirec_mipmaps(
    re_cubemap_t *cubemap,
    const char **paths,
    const uint32_t path_count,
    const uint32_t width,
    const uint32_t height) {
  cubemap->width = width;
  cubemap->height = height;

  cubemap->mip_levels = path_count;

  create_cubemap_image(
      &cubemap->image,
      &cubemap->allocation,
      &cubemap->image_view,
      &cubemap->sampler,
      VK_FORMAT_R32G32B32A32_SFLOAT,
      width,
      height,
      path_count);

  for (size_t i = 0; i < path_count; i++) {
    bake_cubemap(
        paths[i],
        cubemap->image,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        width / pow(2, i),
        height / pow(2, i),
        i);
  }

  cubemap->descriptor = VkDescriptorImageInfo{
      cubemap->sampler,
      cubemap->image_view,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };
}

void re_cubemap_destroy(re_cubemap_t *cubemap) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  if (cubemap->image != VK_NULL_HANDLE) {
    vkDestroyImageView(g_ctx.device, cubemap->image_view, NULL);
    vkDestroySampler(g_ctx.device, cubemap->sampler, NULL);
    vmaDestroyImage(g_ctx.gpu_allocator, cubemap->image, cubemap->allocation);

    cubemap->image = VK_NULL_HANDLE;
    cubemap->allocation = VK_NULL_HANDLE;
    cubemap->image_view = VK_NULL_HANDLE;
    cubemap->sampler = VK_NULL_HANDLE;
  }
}
