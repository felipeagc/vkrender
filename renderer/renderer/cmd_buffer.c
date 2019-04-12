#include "cmd_buffer.h"

#include "context.h"

void re_allocate_cmd_buffers(
    re_cmd_buffer_alloc_info_t alloc_info, re_cmd_buffer_t *buffers) {
  VK_CHECK(vkAllocateCommandBuffers(
      g_ctx.device,
      &(VkCommandBufferAllocateInfo){
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
          .pNext = NULL,
          .commandPool = alloc_info.pool,
          .level = (VkCommandBufferLevel)alloc_info.level,
          .commandBufferCount = alloc_info.count,
      },
      buffers));
}

void re_free_cmd_buffers(
    re_cmd_pool_t pool, uint32_t buffer_count, re_cmd_buffer_t *buffers) {
  vkFreeCommandBuffers(g_ctx.device, pool, buffer_count, buffers);
}

void re_begin_cmd_buffer(
    re_cmd_buffer_t cmd_buffer, re_cmd_buffer_begin_info_t *begin_info) {
  VK_CHECK(vkBeginCommandBuffer(
      cmd_buffer,
      &(VkCommandBufferBeginInfo){
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
          .pNext = NULL,
          .flags = begin_info->usage,
          .pInheritanceInfo = NULL,
      }));
}

void re_end_cmd_buffer(re_cmd_buffer_t cmd_buffer) {
  VK_CHECK(vkEndCommandBuffer(cmd_buffer));
}

/*
 *
 * Commands
 *
 */

void re_cmd_bind_graphics_pipeline(
    re_cmd_buffer_t cmd_buffer, re_pipeline_t *pipeline) {
  vkCmdBindPipeline(
      cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
}
