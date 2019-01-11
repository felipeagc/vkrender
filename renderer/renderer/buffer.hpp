#pragma once

#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

struct re_buffer_t {
  VkBuffer buffer = VK_NULL_HANDLE;
  VmaAllocation allocation = VK_NULL_HANDLE;
};

void re_buffer_init_vertex(re_buffer_t *buffer, size_t size);

void re_buffer_init_index(re_buffer_t *buffer, size_t size);

void re_buffer_init_uniform(re_buffer_t *buffer, size_t size);

void re_buffer_init_staging(re_buffer_t *buffer, size_t size);

bool re_buffer_map_memory(re_buffer_t *buffer, void **dest);

void re_buffer_unmap_memory(re_buffer_t *buffer);

void re_buffer_transfer_to_buffer(
    re_buffer_t *buffer, re_buffer_t *dest, size_t size);

void re_buffer_transfer_to_image(
    re_buffer_t *buffer, VkImage dest, uint32_t width, uint32_t height);

void re_buffer_destroy(re_buffer_t *buffer);
