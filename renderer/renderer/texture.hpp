#pragma once

#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

typedef enum re_format_t {
  RE_FORMAT_RGBA8_UNORM,
  RE_FORMAT_R16_SFLOAT,
} re_format_t;

typedef struct re_texture_t {
  VkImage image;
  VmaAllocation allocation;
  VkImageView image_view;
  VkSampler sampler;

  VkDescriptorImageInfo descriptor;

  uint32_t width;
  uint32_t height;
} re_texture_t;

bool re_texture_init_from_path(
    re_texture_t *texture, const char *path);

void re_texture_init(
    re_texture_t *texture,
    const uint8_t *data,
    const size_t data_size,
    const uint32_t width,
    const uint32_t height,
    re_format_t format);

void re_texture_destroy(re_texture_t *texture);
