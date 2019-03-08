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

static void create_cubemap_image(
    VkImage *image,
    VmaAllocation *allocation,
    VkImageView *imageView,
    VkSampler *sampler,
    VkFormat format,
    uint32_t width,
    uint32_t height,
    uint32_t levels) {
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

static inline void upload_image_to_cubemap(
    re_cubemap_t *cubemap,
    float *data,
    uint32_t width,
    uint32_t height,
    uint32_t layer,
    uint32_t level) {
  // Upload data to image
  size_t img_size = width * height * 4 * sizeof(float);

  re_buffer_t staging_buffer;
  re_buffer_init_staging(&staging_buffer, img_size);

  void *staging_memory_pointer;
  re_buffer_map_memory(&staging_buffer, &staging_memory_pointer);
  memcpy(staging_memory_pointer, data, img_size);

  re_buffer_transfer_to_image(
      &staging_buffer, cubemap->image, width, height, layer, level);

  re_buffer_destroy(&staging_buffer);
}

void re_cubemap_init_from_hdr_sides(re_cubemap_t *cubemap, char *paths[6]) {
  int width, height, nr_comps;
  float *image_datas[6];
  for (uint32_t i = 0; i < 6; i++) {
    image_datas[i] = stbi_loadf(paths[i], &width, &height, &nr_comps, 4);
  }

  cubemap->width = (uint32_t)width;
  cubemap->height = (uint32_t)height;
  cubemap->mip_levels = 1;

  create_cubemap_image(
      &cubemap->image,
      &cubemap->allocation,
      &cubemap->image_view,
      &cubemap->sampler,
      VK_FORMAT_R32G32B32A32_SFLOAT,
      (uint32_t)width,
      (uint32_t)height,
      1);

  for (uint32_t i = 0; i < 6; i++) {
    upload_image_to_cubemap(
        cubemap, image_datas[i], cubemap->width, cubemap->height, i, 0);
  }

  for (uint32_t i = 0; i < 6; i++) {
    free(image_datas[i]);
  }

  cubemap->descriptor = VkDescriptorImageInfo{
      cubemap->sampler,
      cubemap->image_view,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };
}

void re_cubemap_init_from_hdr_sides_with_mip_maps(
    re_cubemap_t *cubemap, char **paths[6], const uint32_t mip_map_levels) {
  uint32_t largest_width = 0, largest_height = 0;
  int width, height, nr_comps;
  float **image_datas[6];
  for (uint32_t i = 0; i < 6; i++) {
    image_datas[i] = (float **)malloc(sizeof(float **) * mip_map_levels);
    for (uint32_t level = 0; level < mip_map_levels; level++) {
      image_datas[i][level] =
          stbi_loadf(paths[i][level], &width, &height, &nr_comps, 4);
      if ((uint32_t)width > largest_width) {
        largest_width = (uint32_t)width;
      }
      if ((uint32_t)height > largest_height) {
        largest_height = (uint32_t)height;
      }
    }
  }

  cubemap->width = largest_width;
  cubemap->height = largest_height;

  cubemap->mip_levels = mip_map_levels;

  create_cubemap_image(
      &cubemap->image,
      &cubemap->allocation,
      &cubemap->image_view,
      &cubemap->sampler,
      VK_FORMAT_R32G32B32A32_SFLOAT,
      cubemap->width,
      cubemap->height,
      cubemap->mip_levels);

  for (uint32_t i = 0; i < 6; i++) {
    for (uint32_t level = 0; level < cubemap->mip_levels; level++) {
      upload_image_to_cubemap(
          cubemap,
          image_datas[i][level],
          cubemap->width / pow(2, level),
          cubemap->height / pow(2, level),
          i,
          level);
    }
  }

  for (uint32_t i = 0; i < 6; i++) {
    for (uint32_t level = 0; level < mip_map_levels; level++) {
      free(image_datas[i][level]);
    }
    free(image_datas[i]);
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
