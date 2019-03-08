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

void re_cubemap_init_from_hdr_sides(
    re_cubemap_t *cubemap, char *paths[6]);

void re_cubemap_init_from_hdr_sides_with_mip_maps(
    re_cubemap_t *cubemap,
    char **paths[6],
    const uint32_t mip_map_levels);

void re_cubemap_destroy(re_cubemap_t *cubemap);
