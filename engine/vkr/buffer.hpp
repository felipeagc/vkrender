#pragma once

#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace vkr {
class Context;

using BufferUsageFlagBits = vk::BufferUsageFlagBits;
using BufferUsageFlags = vk::BufferUsageFlags;

using MemoryPropertyFlagBits = vk::MemoryPropertyFlagBits;
using MemoryPropertyFlags = vk::MemoryPropertyFlags;

enum class MemoryUsageFlagBits {
  eUnknown = VMA_MEMORY_USAGE_UNKNOWN,
  eGpuOnly = VMA_MEMORY_USAGE_GPU_ONLY,
  eCpuOnly = VMA_MEMORY_USAGE_CPU_ONLY,
  eCpuToGpu = VMA_MEMORY_USAGE_CPU_TO_GPU,
  eGpuToCpu = VMA_MEMORY_USAGE_GPU_TO_CPU,
};

using MemoryUsageFlags = vk::Flags<MemoryUsageFlagBits, VmaMemoryUsage>;

class StagingBuffer;
class CommandBuffer;

class Buffer {
  friend class StagingBuffer;
  friend class CommandBuffer;

public:
  Buffer(
      const Context &context,
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

  void destroy();

protected:
  const Context &context;

  vk::Buffer buffer;
  VmaAllocation allocation;
};

class StagingBuffer : public Buffer {
public:
  StagingBuffer(const Context &context, size_t size);
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
