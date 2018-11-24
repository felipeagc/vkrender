#pragma once

#include <vector>
#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace vkr {
class Texture {
public:
  void loadFromPath(const std::string_view &path);
  void loadFromBinary(
      const std::vector<unsigned char> &data,
      const uint32_t width,
      const uint32_t height);

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
} // namespace vkr
