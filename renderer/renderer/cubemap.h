#pragma once

#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

typedef struct re_cubemap_t {
  VkImage image;
  VmaAllocation allocation;
  VkImageView image_view;
  VkSampler sampler;

  VkDescriptorImageInfo descriptor;

  uint32_t width;
  uint32_t height;

  uint32_t mip_levels;
} re_cubemap_t;

/*
 * layers: float* layers[level_count][layer_count]
 * or float* layers[level_count * layer_count]
 */
void re_cubemap_init_from_data(
    re_cubemap_t *cubemap,
    float **layers,
    uint32_t layer_count,
    uint32_t mip_levels,
    uint32_t width,
    uint32_t height);

void re_cubemap_destroy(re_cubemap_t *cubemap);
