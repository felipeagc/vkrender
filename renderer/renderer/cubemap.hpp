#pragma once

#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

typedef struct re_cubemap_t {
  VkImage image;
  VmaAllocation allocation;
  VkImageView image_view;
  VkSampler sampler;

  uint32_t width;
  uint32_t height;

  uint32_t mip_levels;
} re_cubemap_t;

void re_cubemap_init_from_hdr_equirec(
    re_cubemap_t *cubemap,
    const char *path,
    const uint32_t width,
    const uint32_t height);

void re_cubemap_init_from_hdr_equirec_mipmaps(
    re_cubemap_t *cubemap,
    const char **paths,
    const uint32_t path_count,
    const uint32_t width,
    const uint32_t height);

VkDescriptorImageInfo re_cubemap_descriptor(const re_cubemap_t *cubemap);

void re_cubemap_destroy(re_cubemap_t *cubemap);
