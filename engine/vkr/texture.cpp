#include "texture.hpp"
#include "buffer.hpp"
#include "context.hpp"
#include "logging.hpp"
#include <stb_image.h>

using namespace vkr;

Texture::Texture(const std::string_view &path) {
  log::debug("Loading texture: {}", path);

  auto pixels = this->loadImage(path);

  this->createImage();

  Unique<StagingBuffer> stagingBuffer(pixels.size());
  stagingBuffer->copyMemory(pixels.data(), pixels.size());
  stagingBuffer->transfer(this->image, this->width, this->height);
}

Texture::Texture(
    const std::vector<unsigned char> &data,
    const uint32_t width,
    const uint32_t height)
    : width(width), height(height) {
  this->createImage();

  Unique<StagingBuffer> stagingBuffer(data.size());
  stagingBuffer->copyMemory(data.data(), data.size());
  stagingBuffer->transfer(this->image, this->width, this->height);
}

vk::Sampler Texture::getSampler() const { return this->sampler; }

vk::ImageView Texture::getImageView() const { return this->imageView; }

DescriptorImageInfo Texture::getDescriptorInfo() const {
  return {
      this->sampler, this->imageView, vk::ImageLayout::eShaderReadOnlyOptimal};
}

void Texture::destroy() {
  Context::getDevice().waitIdle();
  Context::getDevice().destroy(this->imageView);
  Context::getDevice().destroy(this->sampler);
  vmaDestroyImage(Context::get().allocator, this->image, this->allocation);
}

void Texture::createImage() {
  vk::ImageCreateInfo imageCreateInfo{
      {},                         // flags
      vk::ImageType::e2D,         // imageType
      vk::Format::eR8G8B8A8Unorm, // format
      {
          this->width,             // width
          this->height,            // height
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
      Context::get().allocator,
      reinterpret_cast<VkImageCreateInfo *>(&imageCreateInfo),
      &imageAllocCreateInfo,
      reinterpret_cast<VkImage *>(&this->image),
      &this->allocation,
      nullptr);

  vk::ImageViewCreateInfo imageViewCreateInfo{
      {},                         // flags
      this->image,                // image
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

  this->imageView = Context::getDevice().createImageView(imageViewCreateInfo);

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

  this->sampler = Context::getDevice().createSampler(samplerCreateInfo);
}

std::vector<unsigned char> Texture::loadImage(const std::string_view &path) {
  int width, height, channels;
  stbi_uc *pixels =
      stbi_load(path.data(), &width, &height, &channels, STBI_rgb_alpha);

  vk::DeviceSize imageSize = width * height * 4;

  this->width = static_cast<uint32_t>(width);
  this->height = static_cast<uint32_t>(height);

  if (!pixels) {
    throw std::runtime_error("Failed to load image from disk");
  }

  std::vector<unsigned char> result(pixels, pixels + imageSize);

  stbi_image_free(pixels);

  return result;
}
