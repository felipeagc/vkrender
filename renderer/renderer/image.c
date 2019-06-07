#include "image.h"
#include "buffer.h"
#include "context.h"
#include "util.h"
#include <math.h>
#include <string.h>

void re_image_init(re_image_t *image, re_image_options_t *options) {
  if (options->layer_count == 0) {
    options->layer_count = 1;
  }

  if (options->mip_level_count == 0) {
    options->mip_level_count = 1;
  }

  if (options->format == 0) {
    options->format = VK_FORMAT_R8G8B8A8_UNORM;
  }

  assert(options->width > 0);
  assert(options->height > 0);
  assert(options->mip_level_count > 0);
  assert(options->layer_count > 0);

  image->width = options->width;
  image->height = options->height;
  image->format = options->format;
  image->layer_count = options->layer_count;
  image->mip_level_count = options->mip_level_count;

  // Create image
  {
    VkImageCreateInfo image_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = image->format,
        .extent = {.width = image->width, .height = image->height, .depth = 1},
        .mipLevels = image->mip_level_count,
        .arrayLayers = image->layer_count,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    if (image->layer_count == 6) {
      image_create_info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    VmaAllocationCreateInfo image_alloc_create_info = {0};
    if (options->dedicated) {
      image_alloc_create_info.flags |=
          VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    }
    image_alloc_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VK_CHECK(vmaCreateImage(
        g_ctx.gpu_allocator,
        &image_create_info,
        &image_alloc_create_info,
        &image->image,
        &image->allocation,
        NULL));
  }

  // Create image view
  {
    VkImageViewCreateInfo image_view_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image->image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = image->format,
        .components = {.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                       .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                       .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                       .a = VK_COMPONENT_SWIZZLE_IDENTITY},
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                             .baseMipLevel = 0,
                             .levelCount = image->mip_level_count,
                             .baseArrayLayer = 0,
                             .layerCount = image->layer_count},
    };

    if (image->layer_count == 6) {
      image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    }

    VK_CHECK(vkCreateImageView(
        g_ctx.device, &image_view_create_info, NULL, &image->image_view));
  }

  // Create sampler
  {
    VkSamplerCreateInfo sampler_create_info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 0.0f,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_NEVER,
        .minLod = 0.0f,
        .maxLod = (float)image->mip_level_count,
        .borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };

    if (options->anisotropy) {
      sampler_create_info.anisotropyEnable = VK_TRUE;
      sampler_create_info.maxAnisotropy =
          g_ctx.physical_limits.maxSamplerAnisotropy;
    }

    VK_CHECK(vkCreateSampler(
        g_ctx.device, &sampler_create_info, NULL, &image->sampler));
  }

  image->descriptor = (re_descriptor_info_t){
      .image = {image->sampler,
                image->image_view,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
  };
}

void re_image_upload(
    re_image_t *image,
    re_cmd_pool_t pool,
    uint8_t *data,
    uint32_t width,
    uint32_t height,
    uint32_t level,
    uint32_t layer) {
  assert(
      re_format_pixel_size(image->format) && "I don't know this format's size");

  // Upload data to image
  size_t img_size =
      (size_t)width * (size_t)height * re_format_pixel_size(image->format);

  re_buffer_t staging_buffer;
  re_buffer_init(
      &staging_buffer,
      &(re_buffer_options_t){
          .usage = RE_BUFFER_USAGE_TRANSFER,
          .memory = RE_BUFFER_MEMORY_HOST,
          .size = img_size,
      });

  void *staging_memory_pointer;
  re_buffer_map_memory(&staging_buffer, &staging_memory_pointer);
  memcpy(staging_memory_pointer, data, img_size);

  re_buffer_transfer_to_image(
      &staging_buffer, image->image, pool, width, height, layer, level);

  re_buffer_unmap_memory(&staging_buffer);
  re_buffer_destroy(&staging_buffer);
}

void re_image_destroy(re_image_t *image) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  if (image->image != VK_NULL_HANDLE) {
    vkDestroyImageView(g_ctx.device, image->image_view, NULL);
    vkDestroySampler(g_ctx.device, image->sampler, NULL);
    vmaDestroyImage(g_ctx.gpu_allocator, image->image, image->allocation);

    image->image = VK_NULL_HANDLE;
    image->allocation = VK_NULL_HANDLE;
    image->image_view = VK_NULL_HANDLE;
    image->sampler = VK_NULL_HANDLE;
  }
}
