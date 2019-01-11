#include "buffer.hpp"
#include "context.hpp"
#include "thread_pool.hpp"
#include "util.hpp"
#include <ftl/logging.hpp>

using namespace renderer;

static inline VkCommandBuffer begin_single_time_command_buffer() {
  assert(threadID < VKR_THREAD_COUNT);
  VkCommandBufferAllocateInfo allocateInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      nullptr,
      ctx().m_threadCommandPools[threadID], // commandPool
      VK_COMMAND_BUFFER_LEVEL_PRIMARY,      // level
      1,                                    // commandBufferCount
  };

  VkCommandBuffer command_buffer;

  VK_CHECK(
      vkAllocateCommandBuffers(ctx().m_device, &allocateInfo, &command_buffer));

  VkCommandBufferBeginInfo commandBufferBeginInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      nullptr,
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, // flags
      nullptr,                                     // pInheritanceInfo
  };

  VK_CHECK(vkBeginCommandBuffer(command_buffer, &commandBufferBeginInfo));

  return command_buffer;
}

static inline void
end_single_time_command_buffer(VkCommandBuffer command_buffer) {
  assert(threadID < VKR_THREAD_COUNT);
  VK_CHECK(vkEndCommandBuffer(command_buffer));

  VkSubmitInfo submitInfo{
      VK_STRUCTURE_TYPE_SUBMIT_INFO,
      nullptr,
      0,               // waitSemaphoreCount
      nullptr,         // pWaitSemaphores
      nullptr,         // pWaitDstStageMask
      1,               // commandBufferCount
      &command_buffer, // pCommandBuffers
      0,               // signalSemaphoreCount
      nullptr,         // pSignalSemaphores
  };

  renderer::ctx().m_queueMutex.lock();
  VK_CHECK(
      vkQueueSubmit(ctx().m_transferQueue, 1, &submitInfo, VK_NULL_HANDLE));

  VK_CHECK(vkQueueWaitIdle(ctx().m_transferQueue));
  renderer::ctx().m_queueMutex.unlock();

  vkFreeCommandBuffers(
      ctx().m_device, ctx().m_threadCommandPools[threadID], 1, &command_buffer);
}

static inline void create_buffer(
    VkBuffer *buffer,
    VmaAllocation *allocation,
    size_t size,
    VkBufferUsageFlags buffer_usage,
    VmaMemoryUsage memory_usage,
    VkMemoryPropertyFlags memory_property) {
  VkBufferCreateInfo bufferCreateInfo = {
      VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      nullptr,
      0,                         // flags
      size,                      // size
      buffer_usage,              // usage
      VK_SHARING_MODE_EXCLUSIVE, // sharingMode
      0,                         // queueFamilyIndexCount
      nullptr                    // pQueueFamilyIndices
  };

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = memory_usage;
  allocInfo.requiredFlags = memory_property;

  VK_CHECK(vmaCreateBuffer(
      ctx().m_allocator,
      &bufferCreateInfo,
      &allocInfo,
      buffer,
      allocation,
      nullptr));
}

void re_buffer_init_vertex(re_buffer_t *buffer, size_t size) {
  create_buffer(
      &buffer->buffer,
      &buffer->allocation,
      size,
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void re_buffer_init_index(re_buffer_t *buffer, size_t size) {
  create_buffer(
      &buffer->buffer,
      &buffer->allocation,
      size,
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void re_buffer_init_uniform(re_buffer_t *buffer, size_t size) {
  create_buffer(
      &buffer->buffer,
      &buffer->allocation,
      size,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU,
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void re_buffer_init_staging(re_buffer_t *buffer, size_t size) {
  create_buffer(
      &buffer->buffer,
      &buffer->allocation,
      size,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VMA_MEMORY_USAGE_CPU_ONLY,
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

bool re_buffer_map_memory(re_buffer_t *buffer, void **dest) {
  return vmaMapMemory(ctx().m_allocator, buffer->allocation, dest) ==
         VK_SUCCESS;
}

void re_buffer_unmap_memory(re_buffer_t *buffer) {
  vmaUnmapMemory(ctx().m_allocator, buffer->allocation);
}

void re_buffer_transfer_to_buffer(
    re_buffer_t *buffer, re_buffer_t *dest, size_t size) {
  VkCommandBuffer commandBuffer = begin_single_time_command_buffer();

  VkBufferCopy bufferCopyInfo = {
      0,    // srcOffset
      0,    // dstOffset
      size, // size
  };

  vkCmdCopyBuffer(
      commandBuffer, buffer->buffer, dest->buffer, 1, &bufferCopyInfo);

  end_single_time_command_buffer(commandBuffer);
}

void re_buffer_transfer_to_image(
    re_buffer_t *buffer, VkImage dest, uint32_t width, uint32_t height) {
  VkCommandBuffer commandBuffer = begin_single_time_command_buffer();

  setImageLayout(
      commandBuffer,
      dest,
      VK_IMAGE_ASPECT_COLOR_BIT,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

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
      buffer->buffer,
      dest,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1,
      &region);

  setImageLayout(
      commandBuffer,
      dest,
      VK_IMAGE_ASPECT_COLOR_BIT,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  end_single_time_command_buffer(commandBuffer);
}

void re_buffer_destroy(re_buffer_t *buffer) {
  VK_CHECK(vkDeviceWaitIdle(ctx().m_device));

  if (buffer->buffer != VK_NULL_HANDLE &&
      buffer->allocation != VK_NULL_HANDLE) {
    vmaDestroyBuffer(ctx().m_allocator, buffer->buffer, buffer->allocation);
  }

  buffer->buffer = VK_NULL_HANDLE;
  buffer->allocation = VK_NULL_HANDLE;
}
