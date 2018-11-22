#pragma once

#include <vector>
#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace vkr {
struct Texture {
  VkImage image = VK_NULL_HANDLE;
  VmaAllocation allocation = VK_NULL_HANDLE;
  VkImageView imageView = VK_NULL_HANDLE;
  VkSampler sampler = VK_NULL_HANDLE;

  uint32_t width = 0;
  uint32_t height = 0;

  void loadFromPath(const std::string_view &path);
  void loadFromBinary(
      const std::vector<unsigned char> &data,
      const uint32_t width,
      const uint32_t height);

  operator bool() const;

  VkDescriptorImageInfo getDescriptorInfo() const;

  void destroy();
};
} // namespace vkr
