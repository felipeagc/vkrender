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

using PhysicalDevice = vk::PhysicalDevice;
using Device = vk::Device;
using Image = vk::Image;
using Sampler = vk::Sampler;
using ImageView = vk::ImageView;
using VertexInputRate = vk::VertexInputRate;
using Format = vk::Format;
using IndexType = vk::IndexType;
using DeviceSize = vk::DeviceSize;
using PipelineBindPoint = vk::PipelineBindPoint;
using PipelineLayout = vk::PipelineLayout;
using DescriptorSet = vk::DescriptorSet;
using DescriptorSetLayoutBinding = vk::DescriptorSetLayoutBinding;
using DescriptorPoolSize = vk::DescriptorPoolSize;
using DescriptorType = vk::DescriptorType;
using DescriptorBufferInfo = vk::DescriptorBufferInfo;
using DescriptorImageInfo = vk::DescriptorImageInfo;
using ShaderStageFlagBits = vk::ShaderStageFlagBits;

// Use this wrapper to call .destroy() on destruction
template <class T> class Unique {
public:
  Unique(T &&t) : t(std::move(t)) {}

  template <typename... Args> Unique(Args &&... args) : t(args...) {}

  ~Unique() { t.destroy(); }

  Unique(const Unique &other) = delete;
  Unique(Unique &&other) noexcept = delete;

  Unique &operator=(const Unique &other) = delete;
  Unique &operator=(const Unique &&other) noexcept = delete;

  operator bool() { return t; }

  T *operator->() { return &t; }

  T const *operator->() const { return &t; }

  T &operator*() { return t; }

  T const &operator*() const { return t; }

  const T &get() const { return t; }

  T &get() { return t; }

private:
  T t;
};
} // namespace vkr
