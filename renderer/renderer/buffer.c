#include "buffer.h"
#include "context.h"
#include "task_scheduler.h"
#include "util.h"

static inline VkCommandBuffer begin_single_time_command_buffer() {
  assert(re_worker_id < RE_THREAD_COUNT);
  VkCommandBufferAllocateInfo allocate_info = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      NULL,
      g_ctx.thread_command_pools[re_worker_id], // commandPool
      VK_COMMAND_BUFFER_LEVEL_PRIMARY,          // level
      1,                                        // commandBufferCount
  };

  VkCommandBuffer command_buffer;

  VK_CHECK(
      vkAllocateCommandBuffers(g_ctx.device, &allocate_info, &command_buffer));

  VkCommandBufferBeginInfo command_buffer_begin_info = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      NULL,
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, // flags
      NULL,                                        // pInheritanceInfo
  };

  VK_CHECK(vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info));

  return command_buffer;
}

static inline void
end_single_time_command_buffer(VkCommandBuffer command_buffer) {
  assert(re_worker_id < RE_THREAD_COUNT);
  VK_CHECK(vkEndCommandBuffer(command_buffer));

  VkSubmitInfo submit_info = {
      VK_STRUCTURE_TYPE_SUBMIT_INFO,
      NULL,
      0,               // waitSemaphoreCount
      NULL,            // pWaitSemaphores
      NULL,            // pWaitDstStageMask
      1,               // commandBufferCount
      &command_buffer, // pCommandBuffers
      0,               // signalSemaphoreCount
      NULL,            // pSignalSemaphores
  };

  mtx_lock(&g_ctx.queue_mutex);
  VK_CHECK(
      vkQueueSubmit(g_ctx.transfer_queue, 1, &submit_info, VK_NULL_HANDLE));

  VK_CHECK(vkQueueWaitIdle(g_ctx.transfer_queue));
  mtx_unlock(&g_ctx.queue_mutex);

  vkFreeCommandBuffers(
      g_ctx.device,
      g_ctx.thread_command_pools[re_worker_id],
      1,
      &command_buffer);
}

static inline void create_buffer(
    VkBuffer *buffer,
    VmaAllocation *allocation,
    size_t size,
    VkBufferUsageFlags buffer_usage,
    VmaMemoryUsage memory_usage,
    VkMemoryPropertyFlags memory_property) {
  VkBufferCreateInfo buffer_create_info = {
      VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      NULL,
      0,                         // flags
      size,                      // size
      buffer_usage,              // usage
      VK_SHARING_MODE_EXCLUSIVE, // sharingMode
      0,                         // queueFamilyIndexCount
      NULL                       // pQueueFamilyIndices
  };

  VmaAllocationCreateInfo alloc_info = {};
  alloc_info.usage = memory_usage;
  alloc_info.requiredFlags = memory_property;

  VK_CHECK(vmaCreateBuffer(
      g_ctx.gpu_allocator,
      &buffer_create_info,
      &alloc_info,
      buffer,
      allocation,
      NULL));
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
  return vmaMapMemory(g_ctx.gpu_allocator, buffer->allocation, dest) ==
         VK_SUCCESS;
}

void re_buffer_unmap_memory(re_buffer_t *buffer) {
  vmaUnmapMemory(g_ctx.gpu_allocator, buffer->allocation);
}

void re_buffer_transfer_to_buffer(
    re_buffer_t *buffer, re_buffer_t *dest, size_t size) {
  VkCommandBuffer command_buffer = begin_single_time_command_buffer();

  VkBufferCopy buffer_copy_info = {
      0,    // srcOffset
      0,    // dstOffset
      size, // size
  };

  vkCmdCopyBuffer(
      command_buffer, buffer->buffer, dest->buffer, 1, &buffer_copy_info);

  end_single_time_command_buffer(command_buffer);
}

void re_buffer_transfer_to_image(
    re_buffer_t *buffer,
    VkImage dest,
    uint32_t width,
    uint32_t height,
    uint32_t layer,
    uint32_t level) {
  VkCommandBuffer command_buffer = begin_single_time_command_buffer();

  VkImageSubresourceRange subresource_range = {};
  subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  subresource_range.baseMipLevel = level;
  subresource_range.levelCount = 1;
  subresource_range.baseArrayLayer = layer;
  subresource_range.layerCount = 1;

  re_set_image_layout(
      command_buffer,
      dest,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      subresource_range,
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

  VkBufferImageCopy region = {
      0, // bufferOffset
      0, // bufferRowLength
      0, // bufferImageHeight
      {
          VK_IMAGE_ASPECT_COLOR_BIT, // aspectMask
          level,                     // mipLevel
          layer,                     // baseArrayLayer
          1,                         // layerCount
      },                             // imageSubresource
      {0, 0, 0},                     // imageOffset
      {width, height, 1},            // imageExtent
  };

  vkCmdCopyBufferToImage(
      command_buffer,
      buffer->buffer,
      dest,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1,
      &region);

  re_set_image_layout(
      command_buffer,
      dest,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      subresource_range,
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

  end_single_time_command_buffer(command_buffer);
}

void re_buffer_destroy(re_buffer_t *buffer) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  if (buffer->buffer != VK_NULL_HANDLE &&
      buffer->allocation != VK_NULL_HANDLE) {
    vmaDestroyBuffer(g_ctx.gpu_allocator, buffer->buffer, buffer->allocation);
  }

  buffer->buffer = VK_NULL_HANDLE;
  buffer->allocation = VK_NULL_HANDLE;
}
