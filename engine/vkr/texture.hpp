#pragma once

#include <vulkan/vulkan.hpp>
#include <vulkan/vk_mem_alloc.h>

namespace vkr {
class Texture {
public:
  Texture(const std::string_view &path);
  ~Texture() {};
  Texture(const Texture &other) = default;
  Texture& operator=(Texture &other) = delete;

  vk::Sampler getSampler();
  vk::ImageView getImageView();

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
