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

typedef enum re_image_flags_t {
  RE_IMAGE_FLAG_DEDICATED = 1 << 1,
  RE_IMAGE_FLAG_ANISOTROPY = 1 << 2,
} re_image_flags_t;

typedef enum re_image_usage_t {
  RE_IMAGE_USAGE_SAMPLED = 1 << 1,
  RE_IMAGE_USAGE_TRANSFER_SRC = 1 << 2,
  RE_IMAGE_USAGE_TRANSFER_DST = 1 << 3,
  RE_IMAGE_USAGE_COLOR_ATTACHMENT = 1 << 4,
  RE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT = 1 << 5,
} re_image_usage_t;

typedef enum re_image_aspect_t {
  RE_IMAGE_ASPECT_COLOR = 1 << 1,
  RE_IMAGE_ASPECT_DEPTH = 1 << 2,
  RE_IMAGE_ASPECT_STENCIL = 1 << 3,
} re_image_aspect_t;

typedef struct re_image_options_t {
  re_image_flags_t flags;
  re_image_usage_t usage;
  re_image_aspect_t aspect;
  VkFormat format;
  VkSampleCountFlags sample_count;
  uint32_t width;
  uint32_t height;
  uint32_t layer_count;
  uint32_t mip_level_count;
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

