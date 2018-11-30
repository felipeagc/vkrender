#pragma once

#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace renderer {

enum class BufferType {
  eVertex,
  eIndex,
  eUniform,
  eStaging,
  eOther
};

class Buffer {
public:
  Buffer() {}
  Buffer(BufferType bufferType, size_t size);

  void destroy();

  VkBuffer getHandle() const;

  operator bool() const;

  void mapMemory(void **dest);
  void unmapMemory();

  void bufferTransfer(Buffer &to, size_t size);

  void imageTransfer(VkImage toImage, uint32_t width, uint32_t height);

protected:
  BufferType m_bufferType = BufferType::eOther;
  VkBuffer m_buffer = VK_NULL_HANDLE;
  VmaAllocation m_allocation = VK_NULL_HANDLE;
};
} // namespace renderer
