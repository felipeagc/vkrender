#include "buffer.h"

#include "context.h"
#include "util.h"
#include <string.h>

static inline re_cmd_buffer_t
begin_single_time_command_buffer(re_cmd_pool_t pool) {
  re_cmd_buffer_t cmd_buffer = {0};

  re_allocate_cmd_buffers(
      &(re_cmd_buffer_alloc_info_t){
          .pool  = pool,
          .count = 1,
          .level = RE_CMD_BUFFER_LEVEL_PRIMARY,
      },
      &cmd_buffer);

  re_begin_cmd_buffer(
      &cmd_buffer,
      &(re_cmd_buffer_begin_info_t){
          .usage = RE_CMD_BUFFER_USAGE_ONE_TIME_SUBMIT,
      });

  return cmd_buffer;
}

static inline void end_single_time_command_buffer(
    re_cmd_pool_t pool, re_cmd_buffer_t *cmd_buffer) {
  re_end_cmd_buffer(cmd_buffer);

  VkSubmitInfo submit_info = {
      VK_STRUCTURE_TYPE_SUBMIT_INFO,
      NULL,
      0,                       // waitSemaphoreCount
      NULL,                    // pWaitSemaphores
      NULL,                    // pWaitDstStageMask
      1,                       // commandBufferCount
      &cmd_buffer->cmd_buffer, // pCommandBuffers
      0,                       // signalSemaphoreCount
      NULL,                    // pSignalSemaphores
  };

  mtx_lock(&g_ctx.queue_mutex);
  VK_CHECK(
      vkQueueSubmit(g_ctx.transfer_queue, 1, &submit_info, VK_NULL_HANDLE));

  VK_CHECK(vkQueueWaitIdle(g_ctx.transfer_queue));
  mtx_unlock(&g_ctx.queue_mutex);

  re_free_cmd_buffers(pool, 1, cmd_buffer);
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

  VmaAllocationCreateInfo alloc_info = {0};
  alloc_info.usage                   = memory_usage;
  alloc_info.requiredFlags           = memory_property;

  VK_CHECK(vmaCreateBuffer(
      g_ctx.gpu_allocator,
      &buffer_create_info,
      &alloc_info,
      buffer,
      allocation,
      NULL));
}

void re_buffer_init(re_buffer_t *buffer, re_buffer_options_t *options) {
  assert(options->size > 0);
  assert(options->usage < RE_BUFFER_USAGE_MAX);
  assert(options->memory < RE_BUFFER_MEMORY_MAX);

  VkBufferUsageFlags buffer_usage       = 0;
  VmaMemoryUsage memory_usage           = 0;
  VkMemoryPropertyFlags memory_property = 0;

  switch (options->memory) {
  case RE_BUFFER_MEMORY_HOST: {
    memory_usage    = VMA_MEMORY_USAGE_CPU_TO_GPU;
    memory_property = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    break;
  }
  case RE_BUFFER_MEMORY_DEVICE: {
    memory_usage    = VMA_MEMORY_USAGE_GPU_ONLY;
    memory_property = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    buffer_usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    break;
  }
  default: assert(0);
  }

  switch (options->usage) {
  case RE_BUFFER_USAGE_VERTEX: {
    buffer_usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    break;
  }
  case RE_BUFFER_USAGE_INDEX: {
    buffer_usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    break;
  }
  case RE_BUFFER_USAGE_UNIFORM: {
    buffer_usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    break;
  }
  case RE_BUFFER_USAGE_TRANSFER: {
    buffer_usage |=
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    break;
  }
  default: assert(0);
  }

  buffer->size = options->size;

  create_buffer(
      &buffer->buffer,
      &buffer->allocation,
      options->size,
      buffer_usage,
      memory_usage,
      memory_property);
}

bool re_buffer_map_memory(re_buffer_t *buffer, void **dest) {
  return vmaMapMemory(g_ctx.gpu_allocator, buffer->allocation, dest) ==
         VK_SUCCESS;
}

void re_buffer_unmap_memory(re_buffer_t *buffer) {
  vmaUnmapMemory(g_ctx.gpu_allocator, buffer->allocation);
}

void re_buffer_transfer_to_buffer(
    re_buffer_t *buffer, re_buffer_t *dest, re_cmd_pool_t pool, size_t size) {
  re_cmd_buffer_t cmd_buffer = begin_single_time_command_buffer(pool);

  VkBufferCopy buffer_copy_info = {
      0,    // srcOffset
      0,    // dstOffset
      size, // size
  };

  vkCmdCopyBuffer(
      cmd_buffer.cmd_buffer,
      buffer->buffer,
      dest->buffer,
      1,
      &buffer_copy_info);

  end_single_time_command_buffer(pool, &cmd_buffer);
}

void re_buffer_transfer_to_image(
    re_buffer_t *buffer,
    VkImage dest,
    re_cmd_pool_t pool,
    uint32_t width,
    uint32_t height,
    uint32_t layer,
    uint32_t level) {
  re_cmd_buffer_t cmd_buffer = begin_single_time_command_buffer(pool);

  VkImageSubresourceRange subresource_range = {0};
  subresource_range.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
  subresource_range.baseMipLevel            = level;
  subresource_range.levelCount              = 1;
  subresource_range.baseArrayLayer          = layer;
  subresource_range.layerCount              = 1;

  re_set_image_layout(
      &cmd_buffer,
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
      cmd_buffer.cmd_buffer,
      buffer->buffer,
      dest,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1,
      &region);

  re_set_image_layout(
      &cmd_buffer,
      dest,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      subresource_range,
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

  end_single_time_command_buffer(pool, &cmd_buffer);
}

void re_image_transfer_to_buffer(
    VkImage image,
    re_buffer_t *buffer,
    re_cmd_pool_t pool,
    uint32_t offset_x,
    uint32_t offset_y,
    uint32_t width,
    uint32_t height,
    uint32_t layer,
    uint32_t level) {
  re_cmd_buffer_t command_buffer = begin_single_time_command_buffer(pool);

  VkImageSubresourceRange subresource_range = {
      .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel   = level,
      .levelCount     = 1,
      .baseArrayLayer = layer,
      .layerCount     = 1,
  };

  re_set_image_layout(
      &command_buffer,
      image,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      subresource_range,
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

  VkBufferImageCopy region = {
      .bufferOffset      = 0,
      .bufferRowLength   = 0,
      .bufferImageHeight = 0,
      .imageSubresource  = {.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                           .mipLevel       = level,
                           .baseArrayLayer = layer,
                           .layerCount     = 1},
      .imageOffset       = {offset_x, offset_y, 0},
      .imageExtent       = {width, height, 1},
  };

  vkCmdCopyImageToBuffer(
      command_buffer.cmd_buffer,
      image,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      buffer->buffer,
      1,
      &region);

  re_set_image_layout(
      &command_buffer,
      image,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      subresource_range,
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

  end_single_time_command_buffer(pool, &command_buffer);
}

void re_buffer_destroy(re_buffer_t *buffer) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  if (buffer->buffer != VK_NULL_HANDLE &&
      buffer->allocation != VK_NULL_HANDLE) {
    vmaDestroyBuffer(g_ctx.gpu_allocator, buffer->buffer, buffer->allocation);
  }

  memset(buffer, 0, sizeof(*buffer));
}
