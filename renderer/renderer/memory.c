#include "memory.h"

#include "context.h"
#include <string.h>

static int32_t find_memory_type(
    const VkPhysicalDeviceMemoryProperties *memory_properties,
    uint32_t memory_type_bits_requirement,
    VkMemoryPropertyFlags required_properties) {
  const uint32_t memory_count = memory_properties->memoryTypeCount;
  for (uint32_t memory_index = 0; memory_index < memory_count; ++memory_index) {
    const uint32_t memory_type_bits = (1 << memory_index);
    const bool is_required_memory_type =
        memory_type_bits_requirement & memory_type_bits;

    const VkMemoryPropertyFlags properties =
        memory_properties->memoryTypes[memory_index].propertyFlags;
    const bool has_required_properties =
        (properties & required_properties) == required_properties;

    if (is_required_memory_type && has_required_properties)
      return (int32_t)memory_index;
  }

  // failed to find memory type
  return -1;
}

void re_allocator_init(re_allocator_t *allocator) {}

VkResult re_create_buffer(
    re_allocator_t *allocator,
    VkBufferCreateInfo *create_info,
    re_alloc_info_t *alloc_info,
    VkBuffer *buffer,
    re_allocation_t *allocation) {
  memset(allocation, 0, sizeof(*allocation));

  allocation->size = create_info->size;

  VkResult result = vkCreateBuffer(g_ctx.device, create_info, NULL, buffer);
  if (result != VK_SUCCESS) {
    return result;
  }

  VkMemoryRequirements mem_requirements;
  vkGetBufferMemoryRequirements(g_ctx.device, *buffer, &mem_requirements);

  VkPhysicalDeviceMemoryProperties props = {0};
  vkGetPhysicalDeviceMemoryProperties(g_ctx.physical_device, &props);

  VkMemoryAllocateInfo vk_alloc_info = {0};
  vk_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  vk_alloc_info.allocationSize = mem_requirements.size;

  vk_alloc_info.memoryTypeIndex = find_memory_type(
      &props, mem_requirements.memoryTypeBits, alloc_info->props);

  result =
      vkAllocateMemory(g_ctx.device, &vk_alloc_info, NULL, &allocation->memory);
  if (result != VK_SUCCESS) {
    return result;
  }

  vkBindBufferMemory(
      g_ctx.device, *buffer, allocation->memory, allocation->offset);

  return VK_SUCCESS;
}

VkResult re_map_memory(
    re_allocator_t *allocator, re_allocation_t *allocation, void **dest) {
  return vkMapMemory(
      g_ctx.device,
      allocation->memory,
      allocation->offset,
      allocation->size,
      0,
      dest);
}

void re_unmap_memory(re_allocator_t *allocator, re_allocation_t *allocation) {
  vkUnmapMemory(g_ctx.device, allocation->memory);
}

void re_allocator_destroy(re_allocator_t *allocator) {}

void re_destroy_buffer(
    re_allocator_t *allocator, VkBuffer buffer, re_allocation_t *allocation) {
  vkDestroyBuffer(g_ctx.device, buffer, NULL);
  vkFreeMemory(g_ctx.device, allocation->memory, NULL);
}

