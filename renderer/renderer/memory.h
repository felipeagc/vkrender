#pragma once

#include "vulkan.h"

typedef struct re_alloc_info_t {
  VkMemoryPropertyFlags props;
} re_alloc_info_t;

typedef struct re_allocation_t {
  VkDeviceMemory memory;
  size_t size;
  size_t offset;
} re_allocation_t;

typedef struct re_allocator_t {

} re_allocator_t;

void re_allocator_init(re_allocator_t *allocator);

VkResult re_create_buffer(
    re_allocator_t *allocator,
    VkBufferCreateInfo *create_info,
    re_alloc_info_t *alloc_info,
    VkBuffer *buffer,
    re_allocation_t *allocation);

VkResult re_map_memory(
    re_allocator_t *allocator, re_allocation_t *allocation, void **dest);

void re_unmap_memory(re_allocator_t *allocator, re_allocation_t *allocation);

void re_destroy_buffer(
    re_allocator_t *allocator, VkBuffer buffer, re_allocation_t *allocation);

void re_allocator_destroy(re_allocator_t *allocator);
