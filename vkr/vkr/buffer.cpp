#include "buffer.hpp"
#include "context.hpp"
#include "util.hpp"
#include <fstl/logging.hpp>
#include <iostream>

using namespace vkr;

// Buffer
Buffer::Buffer(
    size_t size,
    VkBufferUsageFlags bufferUsage,
    VmaMemoryUsage memoryUsage,
    VkMemoryPropertyFlags memoryProperty) {
  VkBufferCreateInfo bufferCreateInfo = {
      VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      nullptr,
      0,                         // flags
      size,                      // size
      bufferUsage,               // usage
      VK_SHARING_MODE_EXCLUSIVE, // sharingMode
      0,                         // queueFamilyIndexCount
      nullptr                    // pQueueFamilyIndices
  };

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = memoryUsage;
  allocInfo.requiredFlags = memoryProperty;

  VK_CHECK(vmaCreateBuffer(
      ctx::allocator,
      &bufferCreateInfo,
      &allocInfo,
      &this->buffer_,
      &this->allocation_,
      nullptr));
}

void Buffer::mapMemory(void **dest) {
  if (vmaMapMemory(ctx::allocator, this->allocation_, dest) != VK_SUCCESS) {
    throw std::runtime_error("Failed to map image memory");
  }
}

void Buffer::unmapMemory() {
  vmaUnmapMemory(ctx::allocator, this->allocation_);
}

VkBuffer Buffer::getHandle() const { return this->buffer_; }

void Buffer::destroy() {
  VK_CHECK(vkDeviceWaitIdle(ctx::device));
  vmaDestroyBuffer(ctx::allocator, this->buffer_, this->allocation_);
}

// StagingBuffer
StagingBuffer::StagingBuffer(size_t size)
    : Buffer(
          size,
          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
          VMA_MEMORY_USAGE_CPU_ONLY,
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {}

void StagingBuffer::copyMemory(const void *data, size_t size) {
  void *stagingMemoryPointer;
  this->mapMemory(&stagingMemoryPointer);
  memcpy(stagingMemoryPointer, data, size);
  this->unmapMemory();
}

void StagingBuffer::transfer(Buffer &buffer, size_t size) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommandBuffer();

  VkBufferCopy bufferCopyInfo = {
      0,    // srcOffset
      0,    // dstOffset
      size, // size
  };

  vkCmdCopyBuffer(
      commandBuffer, this->buffer_, buffer.getHandle(), 1, &bufferCopyInfo);

  endSingleTimeCommandBuffer(commandBuffer);
}

void StagingBuffer::transfer(VkImage &image, uint32_t width, uint32_t height) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommandBuffer();

  auto transitionImageLayout = [&](VkImageLayout oldLayout,
                                   VkImageLayout newLayout) {
    VkImageMemoryBarrier barrier{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        nullptr,
        {},                      // srcAccessMask (we'll set this later)
        {},                      // dstAccessMask (we'll set this later)
        oldLayout,               // oldLayout
        newLayout,               // newLayout
        VK_QUEUE_FAMILY_IGNORED, // srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED, // dstQueueFamilyIndex
        image,                   // image
        {
            VK_IMAGE_ASPECT_COLOR_BIT, // aspectMask
            0,                         // baseMipLevel
            1,                         // levelCount
            0,                         // baseArrayLayer
            1,                         // layerCount
        },                             // subresourceRange
    };

    VkPipelineStageFlags srcStage;
    VkPipelineStageFlags dstStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

      srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (
        oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
        newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
      throw std::runtime_error("Unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        srcStage,
        dstStage,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);
  };

  transitionImageLayout(
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  VkBufferImageCopy region{
      0, // bufferOffset
      0, // bufferRowLength
      0, // bufferImageHeight
      {
          VK_IMAGE_ASPECT_COLOR_BIT, // aspectMask
          0,                         // mipLevel
          0,                         // baseArrayLayer
          1,                         // layerCount
      },                             // imageSubresource
      {0, 0, 0},                     // imageOffset
      {width, height, 1},            // imageExtent
  };

  vkCmdCopyBufferToImage(
      commandBuffer,
      this->buffer_,
      image,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1,
      &region);

  transitionImageLayout(
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  endSingleTimeCommandBuffer(commandBuffer);
}

VkCommandBuffer StagingBuffer::beginSingleTimeCommandBuffer() {
  VkCommandBufferAllocateInfo allocateInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      nullptr,
      ctx::transientCommandPool,       // commandPool
      VK_COMMAND_BUFFER_LEVEL_PRIMARY, // level
      1,                               // commandBufferCount
  };

  VkCommandBuffer commandBuffer;

  VK_CHECK(
      vkAllocateCommandBuffers(ctx::device, &allocateInfo, &commandBuffer));

  VkCommandBufferBeginInfo commandBufferBeginInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      nullptr,
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, // flags
      nullptr,                                     // pInheritanceInfo
  };

  VK_CHECK(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

  return commandBuffer;
}

void StagingBuffer::endSingleTimeCommandBuffer(VkCommandBuffer commandBuffer) {
  VK_CHECK(vkEndCommandBuffer(commandBuffer));

  VkSubmitInfo submitInfo{
      VK_STRUCTURE_TYPE_SUBMIT_INFO,
      nullptr,
      0,              // waitSemaphoreCount
      nullptr,        // pWaitSemaphores
      nullptr,        // pWaitDstStageMask
      1,              // commandBufferCount
      &commandBuffer, // pCommandBuffers
      0,              // signalSemaphoreCount
      nullptr,        // pSignalSemaphores
  };

  VK_CHECK(vkQueueSubmit(ctx::transferQueue, 1, &submitInfo, VK_NULL_HANDLE));

  VK_CHECK(vkQueueWaitIdle(ctx::transferQueue));

  vkFreeCommandBuffers(
      ctx::device, ctx::transientCommandPool, 1, &commandBuffer);
}
