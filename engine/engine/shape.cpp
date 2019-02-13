#include "shape.hpp"
#include <renderer/window.hpp>
#include <string.h>

void eg_shape_init(
    eg_shape_t *shape,
    re_vertex_t *vertices,
    uint32_t vertex_count,
    uint32_t *indices,
    uint32_t index_count) {
  shape->index_count = index_count;

  size_t vertex_buffer_size = sizeof(re_vertex_t) * vertex_count;
  size_t index_buffer_size = sizeof(uint32_t) * index_count;
  re_buffer_init_vertex(&shape->vertex_buffer, vertex_buffer_size);
  re_buffer_init_index(&shape->index_buffer, index_buffer_size);

  re_buffer_t staging_buffer;
  re_buffer_init_staging(
      &staging_buffer,
      vertex_buffer_size > index_buffer_size ? vertex_buffer_size
                                             : index_buffer_size);

  void *memory;
  re_buffer_map_memory(&staging_buffer, &memory);

  memcpy(memory, vertices, vertex_buffer_size);
  re_buffer_transfer_to_buffer(
      &staging_buffer, &shape->vertex_buffer, vertex_buffer_size);

  memcpy(memory, indices, index_buffer_size);
  re_buffer_transfer_to_buffer(
      &staging_buffer, &shape->index_buffer, index_buffer_size);

  re_buffer_unmap_memory(&staging_buffer);

  re_buffer_destroy(&staging_buffer);
}

void eg_shape_draw(eg_shape_t *shape, struct re_window_t *window) {
  VkCommandBuffer command_buffer = re_window_get_current_command_buffer(window);

  vkCmdBindIndexBuffer(
      command_buffer, shape->index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
  VkDeviceSize offsets = 0;
  vkCmdBindVertexBuffers(
      command_buffer, 0, 1, &shape->vertex_buffer.buffer, &offsets);
  vkCmdDrawIndexed(command_buffer, shape->index_count, 1, 0, 0, 0);
}

void eg_shape_destroy(eg_shape_t *shape) {
  re_buffer_destroy(&shape->vertex_buffer);
  re_buffer_destroy(&shape->index_buffer);
}
