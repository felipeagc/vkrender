#pragma once

#include "util.hpp"
#include <vulkan/vulkan.hpp>

namespace vkr {
class Context;

class StagingBuffer;
class CommandBuffer;

class Buffer {
public:
  Buffer(
      size_t size,
      BufferUsageFlags bufferUsage,
      MemoryUsageFlags memoryUsage = MemoryUsageFlagBits::eGpuOnly,
      MemoryPropertyFlags memoryProperty = {});
  ~Buffer() {
  }
  Buffer(const Buffer &other) = default;
  Buffer &operator=(const Buffer &other) = default;

  void mapMemory(void **dest);
  void unmapMemory();

  vk::Buffer getVkBuffer() const;

  void destroy();

protected:
  vk::Buffer buffer;
  VmaAllocation allocation;
};

class StagingBuffer : public Buffer {
public:
  StagingBuffer(size_t size);
  ~StagingBuffer() {
  }
  StagingBuffer(const StagingBuffer &other) = default;
  StagingBuffer &operator=(const StagingBuffer &other) = default;

  // Copies memory into the staging buffer
  void copyMemory(void *data, size_t size);

  // Issues a command to transfer this buffer's memory into a vertex buffer
  void transfer(Buffer &buffer, size_t size);

  // Issues a command to transfer this buffer's memory into an index buffer
  // void transfer(IndexBuffer &buffer, size_t size);

  // Issues a command to transfer this buffer's memory into a uniform buffer
  // void transfer(UniformBuffer &buffer, size_t size);

  // Issues a command to transfer this buffer's memory into a texture's image
  // void transfer(Texture &texture);

protected:
  void innerBufferTransfer(
      Buffer &buffer,
      size_t size,
      vk::AccessFlags dstAccessMask,
      vk::PipelineStageFlags dstStageMask);
};
} // namespace vkr
