#pragma once

#include <string>
#include <vector>
#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace renderer {
class Texture {
public:
  Texture(){};
  Texture(const std::string &path);
  Texture(
      const std::vector<unsigned char> &data,
      const uint32_t width,
      const uint32_t height);
  ~Texture(){};

  // Texture can be copied
  Texture(const Texture &) = default;
  Texture &operator=(const Texture &) = default;

  // Texture can be moved
  Texture(Texture &&) = default;
  Texture &operator=(Texture &&) = default;

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
