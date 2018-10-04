#pragma once

#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace vkr {
class Context;

using BufferUsage = vk::BufferUsageFlagBits;
using MemoryProperty = vk::MemoryPropertyFlagBits;

enum MemoryUsage {
  eUnknown = VMA_MEMORY_USAGE_UNKNOWN,
  eGpuOnly = VMA_MEMORY_USAGE_GPU_ONLY,
  eCpuOnly = VMA_MEMORY_USAGE_CPU_ONLY,
  eCpuToGpu = VMA_MEMORY_USAGE_CPU_TO_GPU,
  eGpuToCpu = VMA_MEMORY_USAGE_GPU_TO_CPU,
};

class StagingBuffer;
class CommandBuffer;

class Buffer {
  friend class StagingBuffer;
  friend class CommandBuffer;

public:
  Buffer(
      const Context &context,
      size_t size,
      BufferUsage bufferUsage,
      MemoryUsage memoryUsage = MemoryUsage::eGpuOnly,
      MemoryProperty memoryProperty = {});
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
} // namespace vkr
