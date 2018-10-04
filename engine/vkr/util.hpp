#pragma once

#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace vkr {
template <typename T> using ArrayProxy = vk::ArrayProxy<T>;

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

using IndexType = vk::IndexType;

using DeviceSize = vk::DeviceSize;

class Destroyable {
public:
  virtual void destroy() = 0;
};

// T must be a Destroyable (or have a destroy() method)
// Use this class to have self destructing Destroyables
template <class T> class Unique {
public:
  Unique(T t) : t(t){};
  ~Unique() {
    t.destroy();
  }

  Unique(const Unique &other) = delete;
  Unique(Unique &&other) noexcept = delete;

  Unique &operator=(const Unique &other) = delete;
  Unique &operator=(const Unique &&other) noexcept = delete;

  T *operator->() {
    return &t;
  }

  T const *operator->() const {
    return &t;
  }

  T &operator*() {
    return t;
  }

  T const &operator*() const {
    return t;
  }

  const T &get() const {
    return t;
  }

  T &get() {
    return t;
  }

private:
  T t;
};
} // namespace vkr
