#include "texture.hpp"
#include "buffer.hpp"
#include "context.hpp"
#include "util.hpp"
#include <stb_image.h>
#include <string.h>

static inline VkFormat convert_format(re_format_t format) {
  switch (format) {
  case RE_FORMAT_RGBA8_UNORM:
    return VK_FORMAT_R8G8B8A8_UNORM;
  case RE_FORMAT_R16_SFLOAT:
    return VK_FORMAT_R16_SFLOAT;
  default:
    return VK_FORMAT_UNDEFINED;
  }
}

static inline void create_image(
    VkImage *image,
    VmaAllocation *allocation,
    VkImageView *imageView,
    VkSampler *sampler,
    VkFormat format,
    uint32_t width,
    uint32_t height) {
  VkImageCreateInfo imageCreateInfo = {
      VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      NULL,
      0,                // flags
      VK_IMAGE_TYPE_2D, // imageType
      format,           // format
      {
          width,               // width
          height,              // height
          1,                   // depth
      },                       // extent
      1,                       // mipLevels
      1,                       // arrayLayers
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

bool re_texture_init_from_path(re_texture_t *texture, const char *path) {
  int iwidth, iheight, channels;
  stbi_uc *pixels =
      stbi_load(path, &iwidth, &iheight, &channels, STBI_rgb_alpha);

  size_t imageSize = (size_t)(iwidth * iheight * channels);

  uint32_t width = (uint32_t)iwidth;
  uint32_t height = (uint32_t)iheight;

  if (!pixels) {
    return false;
  }

  re_format_t format;

  switch (channels) {
  case 4:
    format = RE_FORMAT_RGBA8_UNORM;
    break;
  default:
    stbi_image_free(pixels);
    return false;
  }

  re_texture_init(texture, pixels, imageSize, width, height, format);

  stbi_image_free(pixels);

  return true;
}

void re_texture_init(
    re_texture_t *texture,
    const uint8_t *data,
    const size_t data_size,
    const uint32_t width,
    const uint32_t height,
    re_format_t format) {
  texture->width = width;
  texture->height = height;

  create_image(
      &texture->image,
      &texture->allocation,
      &texture->image_view,
      &texture->sampler,
      convert_format(format),
      texture->width,
      texture->height);

  re_buffer_t staging_buffer;
  re_buffer_init_staging(&staging_buffer, data_size);

  void *staging_pointer;
  re_buffer_map_memory(&staging_buffer, &staging_pointer);
  memcpy(staging_pointer, data, data_size);

  re_buffer_transfer_to_image(
      &staging_buffer, texture->image, texture->width, texture->height);

  re_buffer_unmap_memory(&staging_buffer);

  re_buffer_destroy(&staging_buffer);

  texture->descriptor = VkDescriptorImageInfo{
      texture->sampler,
      texture->image_view,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };
}

void re_texture_destroy(re_texture_t *texture) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  if (texture->image != VK_NULL_HANDLE) {
    vkDestroyImageView(g_ctx.device, texture->image_view, NULL);
    vkDestroySampler(g_ctx.device, texture->sampler, NULL);
    vmaDestroyImage(g_ctx.gpu_allocator, texture->image, texture->allocation);

    texture->image = VK_NULL_HANDLE;
    texture->allocation = VK_NULL_HANDLE;
    texture->image_view = VK_NULL_HANDLE;
    texture->sampler = VK_NULL_HANDLE;
  }
}
