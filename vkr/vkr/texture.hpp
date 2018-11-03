#pragma once

#include "aliases.hpp"
#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

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

  operator bool() { return this->image; }

  Sampler getSampler() const;
  ImageView getImageView() const;

  DescriptorImageInfo getDescriptorInfo() const;

  void destroy();

protected:
  vk::Image image;
  VmaAllocation allocation;
  vk::ImageView imageView;
  vk::Sampler sampler;

  uint32_t width;
  uint32_t height;

  void createImage();
  std::vector<unsigned char> loadImage(const std::string_view &path);
};
} // namespace vkr
