#include "texture.hpp"
#include "buffer.hpp"
#include "context.hpp"
#include "util.hpp"
#include <fstl/logging.hpp>
#include <stb_image.h>

using namespace vkr;

Texture::Texture(const std::string_view &path) {
  fstl::log::debug("Loading texture: {}", path);

  auto pixels = this->loadImage(path);

  this->createImage();

  VkBuffer stagingBuffer;
  VmaAllocation stagingAllocation;
  buffer::makeStagingBuffer(pixels.size(), &stagingBuffer, &stagingAllocation);

  void *stagingMemoryPointer;
  buffer::mapMemory(stagingAllocation, &stagingMemoryPointer);
  memcpy(stagingMemoryPointer, pixels.data(), pixels.size());
  buffer::imageTransfer(
      stagingBuffer, this->image_, this->width_, this->height_);
  buffer::unmapMemory(stagingAllocation);

  buffer::destroy(stagingBuffer, stagingAllocation);
}

Texture::Texture(
    const std::vector<unsigned char> &data,
    const uint32_t width,
    const uint32_t height)
    : width_(width), height_(height) {
  fstl::log::debug("Loading texture from binary data");

  this->createImage();

  VkBuffer stagingBuffer;
  VmaAllocation stagingAllocation;
  buffer::makeStagingBuffer(data.size(), &stagingBuffer, &stagingAllocation);

  void *stagingMemoryPointer;
  buffer::mapMemory(stagingAllocation, &stagingMemoryPointer);
  memcpy(stagingMemoryPointer, data.data(), data.size());
  buffer::imageTransfer(
      stagingBuffer, this->image_, this->width_, this->height_);
  buffer::unmapMemory(stagingAllocation);

  buffer::destroy(stagingBuffer, stagingAllocation);
}

VkSampler Texture::getSampler() const { return this->sampler_; }

VkImageView Texture::getImageView() const { return this->imageView_; }

VkDescriptorImageInfo Texture::getDescriptorInfo() const {
  return {
      this->sampler_,
      this->imageView_,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };
}

void Texture::destroy() {
  VK_CHECK(vkDeviceWaitIdle(ctx::device));
  vkDestroyImageView(ctx::device, this->imageView_, nullptr);
  vkDestroySampler(ctx::device, this->sampler_, nullptr);
  vmaDestroyImage(ctx::allocator, this->image_, this->allocation_);

  this->image_ = VK_NULL_HANDLE;
  this->allocation_ = VK_NULL_HANDLE;
  this->imageView_ = VK_NULL_HANDLE;
  this->sampler_ = VK_NULL_HANDLE;
}

void Texture::createImage() {
  VkImageCreateInfo imageCreateInfo = {
      VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      nullptr,
      0,                        // flags
      VK_IMAGE_TYPE_2D,         // imageType
      VK_FORMAT_R8G8B8A8_UNORM, // format
      {
          this->width_,        // width
          this->height_,       // height
          1,                   // depth
      },                       // extent
      1,                       // mipLevels
      1,                       // arrayLayers
      VK_SAMPLE_COUNT_1_BIT,   // samples
      VK_IMAGE_TILING_OPTIMAL, // tiling
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, // usage
      VK_SHARING_MODE_EXCLUSIVE, // sharingMode
      0,                         // queueFamilyIndexCount
      nullptr,                   // pQueueFamilyIndices
      VK_IMAGE_LAYOUT_UNDEFINED, // initialLayout
  };

  VmaAllocationCreateInfo imageAllocCreateInfo = {};
  imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  VK_CHECK(vmaCreateImage(
      ctx::allocator,
      &imageCreateInfo,
      &imageAllocCreateInfo,
      &this->image_,
      &this->allocation_,
      nullptr));

  VkImageViewCreateInfo imageViewCreateInfo = {
      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      nullptr,
      0,                        // flags
      this->image_,             // image
      VK_IMAGE_VIEW_TYPE_2D,    // viewType
      VK_FORMAT_R8G8B8A8_UNORM, // format
      {
          VK_COMPONENT_SWIZZLE_IDENTITY, // r
          VK_COMPONENT_SWIZZLE_IDENTITY, // g
          VK_COMPONENT_SWIZZLE_IDENTITY, // b
          VK_COMPONENT_SWIZZLE_IDENTITY, // a
      },                                 // components
      {
          VK_IMAGE_ASPECT_COLOR_BIT, // aspectMask
          0,                         // baseMipLevel
          1,                         // levelCount
          0,                         // baseArrayLayer
          1,                         // layerCount
      },                             // subresourceRange
  };

  VK_CHECK(vkCreateImageView(
      ctx::device, &imageViewCreateInfo, nullptr, &this->imageView_));

  VkSamplerCreateInfo samplerCreateInfo = {
      VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      nullptr,
      0,                                       // flags
      VK_FILTER_LINEAR,                        // magFilter
      VK_FILTER_LINEAR,                        // minFilter
      VK_SAMPLER_MIPMAP_MODE_LINEAR,           // mipmapMode
      VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT, // addressModeU
      VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT, // addressModeV
      VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT, // addressModeW
      0.0f,                                    // mipLodBias
      VK_FALSE,                                // anisotropyEnable
      1.0f,                                    // maxAnisotropy
      VK_FALSE,                                // compareEnable
      VK_COMPARE_OP_NEVER,                     // compareOp
      0.0f,                                    // minLod
      0.0f,                                    // maxLod
      VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK, // borderColor
      VK_FALSE,                                // unnormalizedCoordinates
  };

  VK_CHECK(vkCreateSampler(
      ctx::device, &samplerCreateInfo, nullptr, &this->sampler_));
}

std::vector<unsigned char> Texture::loadImage(const std::string_view &path) {
  int width, height, channels;
  stbi_uc *pixels =
      stbi_load(path.data(), &width, &height, &channels, STBI_rgb_alpha);

  VkDeviceSize imageSize = width * height * 4;

  this->width_ = static_cast<uint32_t>(width);
  this->height_ = static_cast<uint32_t>(height);

  if (!pixels) {
    throw std::runtime_error("Failed to load image from disk");
  }

  std::vector<unsigned char> result(pixels, pixels + imageSize);

  stbi_image_free(pixels);

  return result;
}
