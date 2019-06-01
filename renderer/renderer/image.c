#include "image.h"
#include "buffer.h"
#include "context.h"
#include "util.h"
#include <math.h>
#include <string.h>

static inline void create_image(
    VkImage *image,
    VmaAllocation *allocation,
    VkImageView *image_view,
    VkSampler *sampler,
    VkFormat format,
    uint32_t width,
    uint32_t height,
    uint32_t layers,
    uint32_t levels) {
  VkImageCreateInfo image_create_info = {
      VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      NULL,             // pNext
      0,                // flags
      VK_IMAGE_TYPE_2D, // imageType
      format,           // format
      {
          width,               // width
          height,              // height
          1,                   // depth
      },                       // extent
      levels,                  // mipLevels
      layers,                  // arrayLayers
      VK_SAMPLE_COUNT_1_BIT,   // samples
      VK_IMAGE_TILING_OPTIMAL, // tiling
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, // usage
      VK_SHARING_MODE_EXCLUSIVE, // sharingMode
      0,                         // queueFamilyIndexCount
      NULL,                      // pQueueFamilyIndices
      VK_IMAGE_LAYOUT_UNDEFINED, // initialLayout
  };

  if (layers == 6) {
    image_create_info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
  }

  VmaAllocationCreateInfo image_alloc_create_info = {0};
  // TODO: maybe this flag is not needed:
  image_alloc_create_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
  image_alloc_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  VK_CHECK(vmaCreateImage(
      g_ctx.gpu_allocator,
      &image_create_info,
      &image_alloc_create_info,
      image,
      allocation,
      NULL));

  VkImageViewCreateInfo image_view_create_info = {
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
          levels,                    // levelCount
          0,                         // baseArrayLayer
          layers,                    // layerCount
      },                             // subresourceRange
  };

  if (layers == 6) {
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
  }

  VK_CHECK(vkCreateImageView(
      g_ctx.device, &image_view_create_info, NULL, image_view));

  VkSamplerCreateInfo sampler_create_info = {
      VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      NULL,
      0,                                          // flags
      VK_FILTER_LINEAR,                           // magFilter
      VK_FILTER_LINEAR,                           // minFilter
      VK_SAMPLER_MIPMAP_MODE_LINEAR,              // mipmapMode
      VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,    // addressModeU
      VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,    // addressModeV
      VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,    // addressModeW
      0.0f,                                       // mipLodBias
      VK_TRUE,                                    // anisotropyEnable
      g_ctx.physical_limits.maxSamplerAnisotropy, // maxAnisotropy
      VK_FALSE,                                   // compareEnable
      VK_COMPARE_OP_NEVER,                        // compareOp
      0.0f,                                       // minLod
      (float)levels,                              // maxLod
      VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,    // borderColor
      VK_FALSE,                                   // unnormalizedCoordinates
  };

  VK_CHECK(vkCreateSampler(g_ctx.device, &sampler_create_info, NULL, sampler));
}

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

  create_image(
      &image->image,
      &image->allocation,
      &image->image_view,
      &image->sampler,
      image->format,
      image->width,
      image->height,
      image->layer_count,
      image->mip_level_count);

  image->descriptor =
      (re_descriptor_info_t){.image = {
                                 image->sampler,
                                 image->image_view,
                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                             }};
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
          .type = RE_BUFFER_TYPE_TRANSFER,
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
