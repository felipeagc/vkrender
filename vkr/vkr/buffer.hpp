#pragma once

#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace vkr {
class Context;

class StagingBuffer;
class CommandBuffer;

class Buffer {
public:
  Buffer() {}
  Buffer(
      size_t size,
      VkBufferUsageFlags bufferUsage,
      VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
      VkMemoryPropertyFlags memoryProperty = 0);
  ~Buffer() {}
  Buffer(const Buffer &other) = delete;
  Buffer &operator=(const Buffer &other) = default;
  Buffer(Buffer &&) = default;
  Buffer &operator=(Buffer &&) = default;

  operator bool() { return this->buffer_; }

  void mapMemory(void **dest);
  void unmapMemory();

  VkBuffer getHandle() const;

  void destroy();

protected:
  VkBuffer buffer_;
  VmaAllocation allocation_;
};

class StagingBuffer : public Buffer {
public:
  StagingBuffer(size_t size);
  ~StagingBuffer() {}
  StagingBuffer(const StagingBuffer &other) = delete;
  StagingBuffer &operator=(const StagingBuffer &other) = default;
  StagingBuffer(StagingBuffer &&) = default;
  StagingBuffer &operator=(StagingBuffer &&) = default;

  // Copies memory into the staging buffer
  void copyMemory(const void *data, size_t size);

  // Issues a command to transfer this buffer's memory into a vertex buffer
  void transfer(Buffer &buffer, size_t size);

  // Issues a command to transfer this buffer's memory into a texture's image
  void transfer(VkImage &image, uint32_t width, uint32_t height);

protected:
  VkCommandBuffer beginSingleTimeCommandBuffer();
  void endSingleTimeCommandBuffer(VkCommandBuffer commandBuffer);
};
} // namespace vkr
