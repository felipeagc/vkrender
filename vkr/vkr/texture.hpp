#pragma once

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
  Texture(const Texture &other) = default;
  Texture &operator=(const Texture &other) = default;

  operator bool() { return this->image_; }

  VkSampler getSampler() const;
  VkImageView getImageView() const;

  VkDescriptorImageInfo getDescriptorInfo() const;

  void destroy();

protected:
  VkImage image_;
  VmaAllocation allocation_;
  VkImageView imageView_;
  VkSampler sampler_;

  uint32_t width_;
  uint32_t height_;

  void createImage();
  std::vector<unsigned char> loadImage(const std::string_view &path);
};
} // namespace vkr
