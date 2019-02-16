#include "mesh_asset.hpp"
#include <renderer/window.hpp>
#include <string.h>

void eg_mesh_asset_init(
    eg_mesh_asset_t *mesh,
    re_vertex_t *vertices,
    uint32_t vertex_count,
    uint32_t *indices,
    uint32_t index_count) {
  eg_asset_init(&mesh->asset, (eg_asset_destructor_t)eg_mesh_asset_destroy);
  mesh->index_count = index_count;

  size_t vertex_buffer_size = sizeof(re_vertex_t) * vertex_count;
  size_t index_buffer_size = sizeof(uint32_t) * index_count;
  re_buffer_init_vertex(&mesh->vertex_buffer, vertex_buffer_size);
  re_buffer_init_index(&mesh->index_buffer, index_buffer_size);

  re_buffer_t staging_buffer;
  re_buffer_init_staging(
      &staging_buffer,
      vertex_buffer_size > index_buffer_size ? vertex_buffer_size
                                             : index_buffer_size);

  void *memory;
  re_buffer_map_memory(&staging_buffer, &memory);

  memcpy(memory, vertices, vertex_buffer_size);
  re_buffer_transfer_to_buffer(
      &staging_buffer, &mesh->vertex_buffer, vertex_buffer_size);

  memcpy(memory, indices, index_buffer_size);
  re_buffer_transfer_to_buffer(
      &staging_buffer, &mesh->index_buffer, index_buffer_size);

  re_buffer_unmap_memory(&staging_buffer);

  re_buffer_destroy(&staging_buffer);
}

void eg_mesh_asset_draw(eg_mesh_asset_t *mesh, struct re_window_t *window) {
  VkCommandBuffer command_buffer = re_window_get_current_command_buffer(window);

  vkCmdBindIndexBuffer(
      command_buffer, mesh->index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
  VkDeviceSize offsets = 0;
  vkCmdBindVertexBuffers(
      command_buffer, 0, 1, &mesh->vertex_buffer.buffer, &offsets);
  vkCmdDrawIndexed(command_buffer, mesh->index_count, 1, 0, 0, 0);
}

void eg_mesh_asset_destroy(eg_mesh_asset_t *mesh) {
  re_buffer_destroy(&mesh->vertex_buffer);
  re_buffer_destroy(&mesh->index_buffer);
  eg_asset_destroy(&mesh->asset);
}
