#pragma once

#include <vector>
#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace vkr {
class Texture {
public:
  Texture(){};
  Texture(const std::string_view &path);
  Texture(
      const std::vector<unsigned char> &data,
      const uint32_t width,
      const uint32_t height);
  ~Texture(){};
  Texture(const Texture &) = default;
  Texture &operator=(const Texture &) = default;
  Texture(Texture &&) = default;
  Texture &operator=(Texture &&) = default;

  operator bool() { return this->image_ != VK_NULL_HANDLE; }

  VkSampler getSampler() const;
  VkImageView getImageView() const;

  VkDescriptorImageInfo getDescriptorInfo() const;

  void destroy();

protected:
  VkImage image_{VK_NULL_HANDLE};
  VmaAllocation allocation_{VK_NULL_HANDLE};
  VkImageView imageView_{VK_NULL_HANDLE};
  VkSampler sampler_{VK_NULL_HANDLE};

  uint32_t width_;
  uint32_t height_;

  void createImage();
  std::vector<unsigned char> loadImage(const std::string_view &path);
};
} // namespace vkr
