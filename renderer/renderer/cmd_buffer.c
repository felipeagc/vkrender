#include "cmd_buffer.h"

#include "context.h"
#include "image.h"
#include <string.h>

void re_allocate_cmd_buffers(
    re_cmd_buffer_alloc_info_t *alloc_info, re_cmd_buffer_t *buffers) {
  assert(alloc_info->count <= RE_MAX_CMD_BUFFER_ALLOCATION);

  VkCommandBuffer cmd_buffers[RE_MAX_CMD_BUFFER_ALLOCATION];

  VK_CHECK(vkAllocateCommandBuffers(
      g_ctx.device,
      &(VkCommandBufferAllocateInfo){
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
          .pNext = NULL,
          .commandPool = alloc_info->pool,
          .level = (VkCommandBufferLevel)alloc_info->level,
          .commandBufferCount = alloc_info->count,
      },
      cmd_buffers));

  memset(buffers, 0, sizeof(*buffers) * alloc_info->count);

  for (uint32_t i = 0; i < alloc_info->count; i++) {
    buffers[i].cmd_buffer = cmd_buffers[i];
  }
}

void re_free_cmd_buffers(
    re_cmd_pool_t pool, uint32_t buffer_count, re_cmd_buffer_t *buffers) {
  assert(buffer_count <= RE_MAX_CMD_BUFFER_ALLOCATION);

  VkCommandBuffer cmd_buffers[RE_MAX_CMD_BUFFER_ALLOCATION] = {0};
  for (uint32_t i = 0; i < buffer_count; i++) {
    cmd_buffers[i] = buffers[i].cmd_buffer;
  }

  vkFreeCommandBuffers(g_ctx.device, pool, buffer_count, cmd_buffers);
}

void re_begin_cmd_buffer(
    re_cmd_buffer_t *cmd_buffer, re_cmd_buffer_begin_info_t *begin_info) {
  VK_CHECK(vkBeginCommandBuffer(
      cmd_buffer->cmd_buffer,
      &(VkCommandBufferBeginInfo){
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
          .pNext = NULL,
          .flags = begin_info->usage,
          .pInheritanceInfo = NULL,
      }));
}

void re_end_cmd_buffer(re_cmd_buffer_t *cmd_buffer) {
  VK_CHECK(vkEndCommandBuffer(cmd_buffer->cmd_buffer));
}

/*
 *
 * Commands
 *
 */

void re_cmd_bind_pipeline(
    re_cmd_buffer_t *cmd_buffer, re_pipeline_t *pipeline) {
  assert(pipeline != NULL);
  vkCmdBindPipeline(
      cmd_buffer->cmd_buffer, pipeline->bind_point, pipeline->pipeline);
}

void re_cmd_bind_descriptor_set(
    re_cmd_buffer_t *cmd_buffer, re_pipeline_t *pipeline, uint32_t set_index) {
  re_descriptor_set_allocator_t *allocator =
      pipeline->layout.descriptor_set_allocators[set_index];
  assert(allocator != NULL);

  VkDescriptorSet set =
      re_descriptor_set_allocator_alloc(allocator, cmd_buffer->bindings);
  assert(set != VK_NULL_HANDLE);

  uint32_t dynamic_offset_count = 0;
  if (allocator->layout.uniform_buffer_dynamic_mask != 0) {
    dynamic_offset_count = 1;
  }

  vkCmdBindDescriptorSets(
      cmd_buffer->cmd_buffer,
      pipeline->bind_point,
      pipeline->layout.layout,
      set_index, // firstSet
      1,
      &set,
      dynamic_offset_count,
      &cmd_buffer->dynamic_offset);
}

void re_cmd_bind_descriptor(
    re_cmd_buffer_t *cmd_buffer,
    uint32_t binding,
    re_descriptor_info_t descriptor) {
  assert(binding < RE_MAX_DESCRIPTOR_SET_BINDINGS);
  cmd_buffer->bindings[binding] = descriptor;
}

void re_cmd_bind_image(
    re_cmd_buffer_t *cmd_buffer, uint32_t binding, re_image_t *image) {
  assert(binding < RE_MAX_DESCRIPTOR_SET_BINDINGS);
  cmd_buffer->bindings[binding] = image->descriptor;
}

void *re_cmd_bind_uniform(
    re_cmd_buffer_t *cmd_buffer, uint32_t binding, size_t size) {
  assert(binding < RE_MAX_DESCRIPTOR_SET_BINDINGS);
  return re_buffer_pool_alloc(
      &g_ctx.ubo_pool,
      size,
      &cmd_buffer->bindings[binding],
      &cmd_buffer->dynamic_offset);
}

void re_cmd_draw(
    re_cmd_buffer_t *cmd_buffer,
    uint32_t vertex_count,
    uint32_t instance_count,
    uint32_t first_vertex,
    uint32_t first_instance) {
  vkCmdDraw(
      cmd_buffer->cmd_buffer,
      vertex_count,
      instance_count,
      first_vertex,
      first_instance);
}

void re_cmd_draw_indexed(
    re_cmd_buffer_t *cmd_buffer,
    uint32_t vertex_count,
    uint32_t instance_count,
    uint32_t first_vertex,
    int32_t vertex_offset,
    uint32_t first_instance) {
  vkCmdDrawIndexed(
      cmd_buffer->cmd_buffer,
      vertex_count,
      instance_count,
      first_vertex,
      vertex_offset,
      first_instance);
}

void re_cmd_bind_vertex_buffers(
    re_cmd_buffer_t *cmd_buffer,
    uint32_t first_binding,
    uint32_t binding_count,
    re_buffer_t *buffers,
    const size_t *offsets) {
  VkBuffer vk_buffers[RE_MAX_VERTEX_BUFFER_BINDINGS];
  for (uint32_t i = 0; i < binding_count; i++) {
    vk_buffers[i] = buffers[i].buffer;
  }

  vkCmdBindVertexBuffers(
      cmd_buffer->cmd_buffer,
      first_binding,
      binding_count,
      vk_buffers,
      offsets);
}

void re_cmd_bind_index_buffer(
    re_cmd_buffer_t *cmd_buffer,
    re_buffer_t *buffer,
    size_t offset,
    re_index_type_t index_type) {
  vkCmdBindIndexBuffer(
      cmd_buffer->cmd_buffer, buffer->buffer, offset, (VkIndexType)index_type);
}
