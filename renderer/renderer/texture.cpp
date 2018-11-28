#include "texture.hpp"
#include "buffer.hpp"
#include "context.hpp"
#include "util.hpp"
#include <fstl/logging.hpp>
#include <stb_image.h>

using namespace renderer;

static void createImage(
    VkImage *image,
    VmaAllocation *allocation,
    VkImageView *imageView,
    VkSampler *sampler,
    uint32_t width,
    uint32_t height) {
  VkImageCreateInfo imageCreateInfo = {
      VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      nullptr,
      0,                        // flags
      VK_IMAGE_TYPE_2D,         // imageType
      VK_FORMAT_R8G8B8A8_UNORM, // format
      {
          width,               // width
          height,              // height
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
      ctx().m_allocator,
      &imageCreateInfo,
      &imageAllocCreateInfo,
      image,
      allocation,
      nullptr));

  VkImageViewCreateInfo imageViewCreateInfo = {
      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      nullptr,
      0,                        // flags
      *image,                   // image
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
      ctx().m_device, &imageViewCreateInfo, nullptr, imageView));

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

  VK_CHECK(
      vkCreateSampler(ctx().m_device, &samplerCreateInfo, nullptr, sampler));
}

static std::vector<unsigned char>
loadImage(const std::string_view &path, uint32_t *width, uint32_t *height) {
  int iwidth, iheight, channels;
  stbi_uc *pixels =
      stbi_load(path.data(), &iwidth, &iheight, &channels, STBI_rgb_alpha);

  size_t imageSize = static_cast<size_t>(iwidth * iheight * 4);

  *width = static_cast<uint32_t>(iwidth);
  *height = static_cast<uint32_t>(iheight);

  if (!pixels) {
    throw std::runtime_error("Failed to load image from disk");
  }

  std::vector<unsigned char> result(pixels, pixels + imageSize);

  stbi_image_free(pixels);

  return result;
}

Texture::Texture(const std::string_view &path) {
  uint32_t width, height;
  auto pixels = loadImage(path, &width, &height);

  *this = Texture{pixels, width, height};
}

Texture::Texture(
    const std::vector<unsigned char> &data,
    const uint32_t width,
    const uint32_t height) {
  m_width = width;
  m_height = height;

  createImage(&m_image, &m_allocation, &m_imageView, &m_sampler, width, height);

  VkBuffer stagingBuffer;
  VmaAllocation stagingAllocation;
  buffer::createStagingBuffer(data.size(), &stagingBuffer, &stagingAllocation);

  void *stagingMemoryPointer;
  buffer::mapMemory(stagingAllocation, &stagingMemoryPointer);
  memcpy(stagingMemoryPointer, data.data(), data.size());
  buffer::imageTransfer(
      stagingBuffer, m_image, m_width, m_height);
  buffer::unmapMemory(stagingAllocation);

  buffer::destroy(stagingBuffer, stagingAllocation);
}

Texture::operator bool() const { return m_image != VK_NULL_HANDLE; }

VkDescriptorImageInfo Texture::getDescriptorInfo() const {
  return {
      m_sampler,
      m_imageView,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };
}

void Texture::destroy() {
  VK_CHECK(vkDeviceWaitIdle(ctx().m_device));
  vkDestroyImageView(ctx().m_device, m_imageView, nullptr);
  vkDestroySampler(ctx().m_device, m_sampler, nullptr);
  vmaDestroyImage(ctx().m_allocator, m_image, m_allocation);

  m_image = VK_NULL_HANDLE;
  m_allocation = VK_NULL_HANDLE;
  m_imageView = VK_NULL_HANDLE;
  m_sampler = VK_NULL_HANDLE;
}
