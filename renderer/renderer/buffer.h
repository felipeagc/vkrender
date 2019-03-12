#pragma once

#include <stdbool.h>
#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

typedef enum re_buffer_type_t {
  RE_BUFFER_TYPE_VERTEX,
  RE_BUFFER_TYPE_INDEX,
  RE_BUFFER_TYPE_UNIFORM,
  RE_BUFFER_TYPE_TRANSFER,

  RE_BUFFER_TYPE_MAX,
} re_buffer_type_t;

typedef struct re_buffer_options_t {
  re_buffer_type_t type;
  size_t size;
} re_buffer_options_t;

typedef struct re_buffer_t {
  VkBuffer buffer;
  VmaAllocation allocation;
} re_buffer_t;

void re_buffer_init(re_buffer_t *buffer, re_buffer_options_t *options);

bool re_buffer_map_memory(re_buffer_t *buffer, void **dest);

void re_buffer_unmap_memory(re_buffer_t *buffer);

void re_buffer_transfer_to_buffer(
    re_buffer_t *buffer, re_buffer_t *dest, size_t size);

void re_buffer_transfer_to_image(
    re_buffer_t *buffer,
    VkImage dest,
    uint32_t width,
    uint32_t height,
    uint32_t layer,
    uint32_t level);

void re_buffer_destroy(re_buffer_t *buffer);
