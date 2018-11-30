#include "buffer.hpp"
#include "context.hpp"
#include "util.hpp"
#include <fstl/logging.hpp>

using namespace renderer;

static inline VkCommandBuffer beginSingleTimeCommandBuffer() {
  VkCommandBufferAllocateInfo allocateInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      nullptr,
      ctx().m_transientCommandPool,    // commandPool
      VK_COMMAND_BUFFER_LEVEL_PRIMARY, // level
      1,                               // commandBufferCount
  };

  VkCommandBuffer commandBuffer;

  VK_CHECK(
      vkAllocateCommandBuffers(ctx().m_device, &allocateInfo, &commandBuffer));

  VkCommandBufferBeginInfo commandBufferBeginInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      nullptr,
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, // flags
      nullptr,                                     // pInheritanceInfo
  };

  VK_CHECK(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

  return commandBuffer;
}

static inline void endSingleTimeCommandBuffer(VkCommandBuffer commandBuffer) {
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

  VK_CHECK(
      vkQueueSubmit(ctx().m_transferQueue, 1, &submitInfo, VK_NULL_HANDLE));

  VK_CHECK(vkQueueWaitIdle(ctx().m_transferQueue));

  vkFreeCommandBuffers(
      ctx().m_device, ctx().m_transientCommandPool, 1, &commandBuffer);
}

Buffer::Buffer(BufferType bufferType, size_t size) : m_bufferType(bufferType) {
  VkBufferUsageFlags bufferUsage;
  VmaMemoryUsage memoryUsage;
  VkMemoryPropertyFlags memoryProperty;

  switch (bufferType) {
  case BufferType::eVertex:
    bufferUsage =
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    break;
  case BufferType::eIndex:
    bufferUsage =
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    break;
  case BufferType::eUniform:
    bufferUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    memoryProperty = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    break;
  case BufferType::eStaging:
    bufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY;
    memoryProperty = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    break;
  case BufferType::eOther:
    assert(false);
    break;
  }

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
      ctx().m_allocator,
      &bufferCreateInfo,
      &allocInfo,
      &m_buffer,
      &m_allocation,
      nullptr));
}

void Buffer::destroy() {
  VK_CHECK(vkDeviceWaitIdle(ctx().m_device));

  if (m_buffer != VK_NULL_HANDLE && m_allocation != VK_NULL_HANDLE) {
    vmaDestroyBuffer(ctx().m_allocator, m_buffer, m_allocation);
  }

  m_buffer = VK_NULL_HANDLE;
  m_allocation = VK_NULL_HANDLE;
}

VkBuffer Buffer::getHandle() const { return m_buffer; }

Buffer::operator bool() const { return (m_buffer != VK_NULL_HANDLE); }

void Buffer::mapMemory(void **dest) {
  if (vmaMapMemory(ctx().m_allocator, m_allocation, dest) != VK_SUCCESS) {
    throw std::runtime_error("Failed to map image memory");
  }
}

void Buffer::unmapMemory() { vmaUnmapMemory(ctx().m_allocator, m_allocation); }

void Buffer::bufferTransfer(Buffer &to, size_t size) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommandBuffer();

  VkBufferCopy bufferCopyInfo = {
      0,    // srcOffset
      0,    // dstOffset
      size, // size
  };

  vkCmdCopyBuffer(commandBuffer, m_buffer, to.m_buffer, 1, &bufferCopyInfo);

  endSingleTimeCommandBuffer(commandBuffer);
}

void Buffer::imageTransfer(VkImage toImage, uint32_t width, uint32_t height) {
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
        toImage,                 // image
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
      m_buffer,
      toImage,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1,
      &region);

  transitionImageLayout(
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  endSingleTimeCommandBuffer(commandBuffer);
}
