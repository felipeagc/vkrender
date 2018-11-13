#include "texture.hpp"
#include "buffer.hpp"
#include "context.hpp"
#include <fstl/logging.hpp>
#include <stb_image.h>

using namespace vkr;

Texture::Texture(const std::string_view &path) {
  fstl::log::debug("Loading texture: {}", path);

  auto pixels = this->loadImage(path);

  this->createImage();

  StagingBuffer stagingBuffer(pixels.size());
  stagingBuffer.copyMemory(pixels.data(), pixels.size());
  stagingBuffer.transfer(this->image_, this->width_, this->height_);
  stagingBuffer.destroy();
}

Texture::Texture(
    const std::vector<unsigned char> &data,
    const uint32_t width,
    const uint32_t height)
    : width_(width), height_(height) {
  fstl::log::debug("Loading texture from binary data");

  this->createImage();

  StagingBuffer stagingBuffer(data.size());
  stagingBuffer.copyMemory(data.data(), data.size());
  stagingBuffer.transfer(this->image_, this->width_, this->height_);
  stagingBuffer.destroy();
}

vk::Sampler Texture::getSampler() const { return this->sampler_; }

vk::ImageView Texture::getImageView() const { return this->imageView_; }

DescriptorImageInfo Texture::getDescriptorInfo() const {
  return {
      this->sampler_, this->imageView_, vk::ImageLayout::eShaderReadOnlyOptimal};
}

void Texture::destroy() {
  Context::getDevice().waitIdle();
  Context::getDevice().destroy(this->imageView_);
  Context::getDevice().destroy(this->sampler_);
  vmaDestroyImage(Context::get().allocator_, this->image_, this->allocation_);
}

void Texture::createImage() {
  vk::ImageCreateInfo imageCreateInfo{
      {},                         // flags
      vk::ImageType::e2D,         // imageType
      vk::Format::eR8G8B8A8Unorm, // format
      {
          this->width_,             // width
          this->height_,            // height
          1,                       // depth
      },                           // extent
      1,                           // mipLevels
      1,                           // arrayLayers
      vk::SampleCountFlagBits::e1, // samples
      vk::ImageTiling::eOptimal,   // tiling
      vk::ImageUsageFlagBits::eTransferDst |
          vk::ImageUsageFlagBits::eSampled, // usage
      vk::SharingMode::eExclusive,          // sharingMode
      0,                                    // queueFamilyIndexCount
      nullptr,                              // pQueueFamilyIndices
      vk::ImageLayout::eUndefined,          // initialLayout
  };

  VmaAllocationCreateInfo imageAllocCreateInfo = {};
  imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  vmaCreateImage(
      Context::get().allocator_,
      reinterpret_cast<VkImageCreateInfo *>(&imageCreateInfo),
      &imageAllocCreateInfo,
      reinterpret_cast<VkImage *>(&this->image_),
      &this->allocation_,
      nullptr);

  vk::ImageViewCreateInfo imageViewCreateInfo{
      {},                         // flags
      this->image_,                // image
      vk::ImageViewType::e2D,     // viewType
      vk::Format::eR8G8B8A8Unorm, // format
      {
          vk::ComponentSwizzle::eIdentity, // r
          vk::ComponentSwizzle::eIdentity, // g
          vk::ComponentSwizzle::eIdentity, // b
          vk::ComponentSwizzle::eIdentity, // a
      },                                   // components
      {
          vk::ImageAspectFlagBits::eColor, // aspectMask
          0,                               // baseMipLevel
          1,                               // levelCount
          0,                               // baseArrayLayer
          1,                               // layerCount
      },                                   // subresourceRange
  };

  this->imageView_ = Context::getDevice().createImageView(imageViewCreateInfo);

  vk::SamplerCreateInfo samplerCreateInfo{
      {},                                      // flags
      vk::Filter::eLinear,                     // magFilter
      vk::Filter::eLinear,                     // minFilter
      vk::SamplerMipmapMode::eLinear,          // mipmapMode
      vk::SamplerAddressMode::eMirroredRepeat, // addressModeU
      vk::SamplerAddressMode::eMirroredRepeat, // addressModeV
      vk::SamplerAddressMode::eMirroredRepeat, // addressModeW
      0.0f,                                    // mipLodBias
      VK_FALSE,                                // anisotropyEnable
      1.0f,                                    // maxAnisotropy
      VK_FALSE,                                // compareEnable
      vk::CompareOp::eNever,                   // compareOp
      0.0f,                                    // minLod
      0.0f,                                    // maxLod
      vk::BorderColor::eFloatTransparentBlack, // borderColor
      VK_FALSE,                                // unnormalizedCoordinates
  };

  this->sampler_ = Context::getDevice().createSampler(samplerCreateInfo);
}

std::vector<unsigned char> Texture::loadImage(const std::string_view &path) {
  int width, height, channels;
  stbi_uc *pixels =
      stbi_load(path.data(), &width, &height, &channels, STBI_rgb_alpha);

  vk::DeviceSize imageSize = width * height * 4;

  this->width_ = static_cast<uint32_t>(width);
  this->height_ = static_cast<uint32_t>(height);

  if (!pixels) {
    throw std::runtime_error("Failed to load image from disk");
  }

  std::vector<unsigned char> result(pixels, pixels + imageSize);

  stbi_image_free(pixels);

  return result;
}
