#pragma once

#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace vkr {
namespace buffer {

void makeVertexBuffer(size_t size, VkBuffer *buffer, VmaAllocation *allocation);

void makeIndexBuffer(size_t size, VkBuffer *buffer, VmaAllocation *allocation);

void makeUniformBuffer(
    size_t size, VkBuffer *buffer, VmaAllocation *allocation);

void makeStagingBuffer(
    size_t size, VkBuffer *buffer, VmaAllocation *allocation);

void mapMemory(VmaAllocation allocation, void **dest);

void unmapMemory(VmaAllocation allocation);

void destroy(VkBuffer buffer, VmaAllocation allocation);

void bufferTransfer(VkBuffer from, VkBuffer to, size_t size);

void imageTransfer(
    VkBuffer fromBuffer, VkImage toImage, uint32_t width, uint32_t height);

template <int count> struct Buffers {
  VkBuffer buffers[count];
  VmaAllocation allocations[count];
};

} // namespace buffer
} // namespace vkr
