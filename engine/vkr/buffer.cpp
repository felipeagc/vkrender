#include "buffer.hpp"
#include "context.hpp"

using namespace vkr;

// Buffer
Buffer::Buffer(
    const Context &context,
    size_t size,
    BufferUsage bufferUsage,
    MemoryUsage memoryUsage,
    MemoryProperty requiredFlags)
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

