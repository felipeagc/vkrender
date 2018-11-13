#include "buffer.hpp"
#include "context.hpp"
#include <iostream>

using namespace vkr;

// Buffer
Buffer::Buffer(
    size_t size,
    BufferUsageFlags bufferUsage,
    MemoryUsageFlags memoryUsage,
    MemoryPropertyFlags requiredFlags) {
  vk::BufferCreateInfo bufferCreateInfo = {
      {},                          // flags
      size,                        // size
      bufferUsage,                 // usage
      vk::SharingMode::eExclusive, // sharingMode
      0,                           // queueFamilyIndexCount
      nullptr                      // pQueueFamilyIndices
  };

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = static_cast<VmaMemoryUsage>(memoryUsage);
  allocInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(requiredFlags);

  if (vmaCreateBuffer(
          Context::get().allocator_,
          reinterpret_cast<VkBufferCreateInfo *>(&bufferCreateInfo),
          &allocInfo,
          reinterpret_cast<VkBuffer *>(&this->buffer_),
          &this->allocation_,
          nullptr) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create buffer");
  }
}

void Buffer::mapMemory(void **dest) {
  if (vmaMapMemory(Context::get().allocator_, this->allocation_, dest) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to map image memory");
  }
}

void Buffer::unmapMemory() {
  vmaUnmapMemory(Context::get().allocator_, this->allocation_);
}

vk::Buffer Buffer::getHandle() const {
  return this->buffer_;
}

void Buffer::destroy() {
  Context::getDevice().waitIdle();
  vmaDestroyBuffer(Context::get().allocator_, this->buffer_, this->allocation_);
}

// StagingBuffer
StagingBuffer::StagingBuffer(size_t size)
    : Buffer(
          size,
          BufferUsageFlagBits::eTransferSrc,
          MemoryUsageFlagBits::eCpuOnly,
          MemoryPropertyFlagBits::eHostCoherent) {
}

void StagingBuffer::copyMemory(const void *data, size_t size) {
  void *stagingMemoryPointer;
  this->mapMemory(&stagingMemoryPointer);
  memcpy(stagingMemoryPointer, data, size);
  this->unmapMemory();
}

void StagingBuffer::transfer(Buffer &buffer, size_t size) {
  vk::CommandBuffer commandBuffer = beginSingleTimeCommandBuffer();

  vk::BufferCopy bufferCopyInfo = {
      0,    // srcOffset
      0,    // dstOffset
      size, // size
  };

  commandBuffer.copyBuffer(
      this->buffer_, buffer.getHandle(), 1, &bufferCopyInfo);

  endSingleTimeCommandBuffer(commandBuffer);
}

void StagingBuffer::transfer(Image &image, uint32_t width, uint32_t height) {
  vk::CommandBuffer commandBuffer = beginSingleTimeCommandBuffer();

  auto transitionImageLayout = [&](vk::ImageLayout oldLayout,
                                   vk::ImageLayout newLayout) {
    vk::ImageMemoryBarrier barrier{
        {},                      // srcAccessMask (we'll set this later)
        {},                      // dstAccessMask (we'll set this later)
        oldLayout,               // oldLayout
        newLayout,               // newLayout
        VK_QUEUE_FAMILY_IGNORED, // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED, // dstQueueFamilyIndex
        image,                   // image
        {
            vk::ImageAspectFlagBits::eColor, // aspectMask
            0,                               // baseMipLevel
            1,                               // levelCount
            0,                               // baseArrayLayer
            1,                               // layerCount
        },                                   // subresourceRange
    };

    vk::PipelineStageFlags srcStage;
    vk::PipelineStageFlags dstStage;

    if (oldLayout == vk::ImageLayout::eUndefined &&
        newLayout == vk::ImageLayout::eTransferDstOptimal) {
      barrier.srcAccessMask = {};
      barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

      srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
      dstStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (
        oldLayout == vk::ImageLayout::eTransferDstOptimal &&
        newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
      barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
      barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

      srcStage = vk::PipelineStageFlagBits::eTransfer;
      dstStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else {
      throw std::invalid_argument("Unsupported layout transition!");
    }

    commandBuffer.pipelineBarrier(srcStage, dstStage, {}, {}, {}, barrier);
  };

  transitionImageLayout(
      vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

  vk::BufferImageCopy region{
      0, // bufferOffset
      0, // bufferRowLength
      0, // bufferImageHeight
      {
          vk::ImageAspectFlagBits::eColor, // aspectMask
          0,                               // mipLevel
          0,                               // baseArrayLayer
          1,                               // layerCount
      },                                   // imageSubresource
      {0, 0, 0},                           // imageOffset
      {width, height, 1},                  // imageExtent
  };

  commandBuffer.copyBufferToImage(
      this->buffer_, image, vk::ImageLayout::eTransferDstOptimal, 1, &region);

  transitionImageLayout(
      vk::ImageLayout::eTransferDstOptimal,
      vk::ImageLayout::eShaderReadOnlyOptimal);

  endSingleTimeCommandBuffer(commandBuffer);
}

vk::CommandBuffer StagingBuffer::beginSingleTimeCommandBuffer() {
  vk::CommandBufferAllocateInfo allocateInfo{
      Context::get().transientCommandPool_, // commandPool
      vk::CommandBufferLevel::ePrimary,    // level
      1,                                   // commandBufferCount
  };

  auto commandBuffers =
      Context::getDevice().allocateCommandBuffers(allocateInfo);
  auto commandBuffer = commandBuffers[0];

  vk::CommandBufferBeginInfo commandBufferBeginInfo{
      vk::CommandBufferUsageFlagBits::eOneTimeSubmit, // flags
      nullptr,                                        // pInheritanceInfo
  };

  commandBuffer.begin(commandBufferBeginInfo);

  return commandBuffer;
}

void StagingBuffer::endSingleTimeCommandBuffer(
    vk::CommandBuffer commandBuffer) {
  commandBuffer.end();

  vk::SubmitInfo submitInfo{
      0,              // waitSemaphoreCount
      nullptr,        // pWaitSemaphores
      nullptr,        // pWaitDstStageMask
      1,              // commandBufferCount
      &commandBuffer, // pCommandBuffers
      0,              // signalSemaphoreCount
      nullptr,        // pSignalSemaphores
  };

  Context::get().transferQueue_.submit(submitInfo, {});

  Context::get().transferQueue_.waitIdle();

  Context::getDevice().freeCommandBuffers(
      Context::get().transientCommandPool_, commandBuffer);
}
