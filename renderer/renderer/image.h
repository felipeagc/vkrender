#pragma once

#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

typedef enum re_format_t {
  RE_FORMAT_UNDEFINED,

  RE_FORMAT_R8G8B8_UNORM,
  RE_FORMAT_R8G8B8A8_UNORM,
  RE_FORMAT_R32G32B32A32_SFLOAT,

  RE_FORMAT_MAX,
} re_format_t;

typedef struct re_image_t {
  VkImage image;
  VmaAllocation allocation;
  VkImageView image_view;
  VkSampler sampler;

  VkDescriptorImageInfo descriptor;

  uint32_t width;
  uint32_t height;
  uint32_t layer_count;
  uint32_t mip_level_count;
  re_format_t format;
} re_image_t;

/*
 * data: uint8_t * layers[level_count][layer_count]
 * or uint8_t* layers[level_count * layer_count]
 */
typedef struct re_image_options_t {
  uint8_t *data;
  uint32_t width;
  uint32_t height;
  uint32_t layer_count;
  uint32_t mip_level_count;
  re_format_t format;
} re_image_options_t;

void re_image_init(re_image_t *image, re_image_options_t *options);

void re_image_destroy(re_image_t *image);

