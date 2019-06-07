#pragma once

#include "cmd_buffer.h"
#include "descriptor_set.h"
#include "vulkan.h"
#include <vma/vk_mem_alloc.h>

typedef struct re_image_t {
  VkImage image;
  VmaAllocation allocation;
  VkImageView image_view;
  VkSampler sampler;

  re_descriptor_info_t descriptor;

  uint32_t width;
  uint32_t height;
  uint32_t layer_count;
  uint32_t mip_level_count;
  VkFormat format;
} re_image_t;

typedef struct re_image_options_t {
  uint32_t width;
  uint32_t height;
  uint32_t layer_count;
  uint32_t mip_level_count;
  VkFormat format;
} re_image_options_t;

void re_image_init(re_image_t *image, re_image_options_t *options);

void re_image_upload(
    re_image_t *image,
    re_cmd_pool_t pool,
    uint8_t *data,
    uint32_t width,
    uint32_t height,
    uint32_t level,
    uint32_t layer);

void re_image_destroy(re_image_t *image);

