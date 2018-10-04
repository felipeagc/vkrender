#include "buffer.hpp"
#include "context.hpp"

using namespace vkr;

// Buffer
Buffer::Buffer(
    const Context &context,
    size_t size,
    BufferUsageFlags bufferUsage,
    MemoryUsageFlags memoryUsage,
    MemoryPropertyFlags requiredFlags)
    : context(context) {
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
          this->context.allocator,
          reinterpret_cast<VkBufferCreateInfo *>(&bufferCreateInfo),
          &allocInfo,
          reinterpret_cast<VkBuffer *>(&this->buffer),
          &this->allocation,
          nullptr) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create buffer");
  }
}

void Buffer::mapMemory(void **dest) {
  if (vmaMapMemory(this->context.allocator, this->allocation, dest) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to map image memory");
  }
}

void Buffer::unmapMemory() {
  vmaUnmapMemory(this->context.allocator, this->allocation);
}

void Buffer::destroy() {
  this->context.device.waitIdle();
  vmaDestroyBuffer(this->context.allocator, this->buffer, this->allocation);
}

// StagingBuffer
StagingBuffer::StagingBuffer(const Context &context, size_t size)
    : Buffer(
          context,
          size,
          BufferUsageFlagBits::eTransferSrc,
          MemoryUsageFlagBits::eCpuOnly,
          MemoryPropertyFlagBits::eHostCoherent) {
}

void StagingBuffer::copyMemory(void *data, size_t size) {
  void *stagingMemoryPointer;
  this->mapMemory(&stagingMemoryPointer);
  memcpy(stagingMemoryPointer, data, size);
  this->unmapMemory();
}

void StagingBuffer::innerBufferTransfer(
    Buffer &buffer,
    size_t size,
    vk::AccessFlags dstAccessMask,
    vk::PipelineStageFlags dstStageMask) {
  vk::CommandBufferAllocateInfo allocateInfo{
      this->context.transientCommandPool, // commandPool
      vk::CommandBufferLevel::ePrimary,   // level
      1,                                  // commandBufferCount
  };

  auto commandBuffers =
      this->context.device.allocateCommandBuffers(allocateInfo);
  auto commandBuffer = commandBuffers[0];

  vk::CommandBufferBeginInfo commandBufferBeginInfo{
      vk::CommandBufferUsageFlagBits::eOneTimeSubmit, // flags
      nullptr,                                        // pInheritanceInfo
  };

  commandBuffer.begin(commandBufferBeginInfo);

  vk::BufferCopy bufferCopyInfo = {
      0,    // srcOffset
      0,    // dstOffset
      size, // size
  };

  commandBuffer.copyBuffer(this->buffer, buffer.buffer, 1, &bufferCopyInfo);

  vk::BufferMemoryBarrier bufferMemoryBarrier{
      vk::AccessFlagBits::eMemoryWrite, // srcAccessMask
      dstAccessMask,                    // dstAccessMask
      VK_QUEUE_FAMILY_IGNORED,          // srcQueueFamilyIndex
      VK_QUEUE_FAMILY_IGNORED,          // dstQueueFamilyIndex
      buffer.buffer,                    // buffer
      0,                                // offset
      VK_WHOLE_SIZE,                    // size
  };

  commandBuffer.pipelineBarrier(
      vk::PipelineStageFlagBits::eTransfer,
      dstStageMask,
      {},
      {},
      bufferMemoryBarrier,
      {});

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

  this->context.transferQueue.submit(submitInfo, {});

  this->context.transferQueue.waitIdle();

  this->context.device.freeCommandBuffers(
      this->context.transientCommandPool, commandBuffer);
}

void StagingBuffer::transfer(Buffer &buffer, size_t size) {
  this->innerBufferTransfer(
      buffer,
      size,
      vk::AccessFlagBits::eVertexAttributeRead,
      vk::PipelineStageFlagBits::eVertexInput);
}
