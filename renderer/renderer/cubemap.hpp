#pragma once

#include <array>
#include <string>
#include <vector>
#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace renderer {
class Cubemap {
public:
  Cubemap(){};
  Cubemap(
      const std::string &hdrPath, const uint32_t width, const uint32_t height);
  ~Cubemap(){};

  // Cubemap can be copied
  Cubemap(const Cubemap &) = default;
  Cubemap &operator=(const Cubemap &) = default;

  // Cubemap can be moved
  Cubemap(Cubemap &&) = default;
  Cubemap &operator=(Cubemap &&) = default;

  operator bool() const;

  VkDescriptorImageInfo getDescriptorInfo() const;

  void destroy();

private:
  VkImage m_image = VK_NULL_HANDLE;
  VmaAllocation m_allocation = VK_NULL_HANDLE;
  VkImageView m_imageView = VK_NULL_HANDLE;
  VkSampler m_sampler = VK_NULL_HANDLE;

  uint32_t m_width = 0;
  uint32_t m_height = 0;
};
} // namespace renderer
