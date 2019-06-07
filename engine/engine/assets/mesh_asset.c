#include "mesh_asset.h"
#include <renderer/context.h>
#include <renderer/window.h>
#include <string.h>

void eg_mesh_asset_inspect(eg_mesh_asset_t *mesh, eg_inspector_t *inspector) {}

void eg_mesh_asset_destroy(eg_mesh_asset_t *mesh) {
  re_buffer_destroy(&mesh->vertex_buffer);
  re_buffer_destroy(&mesh->index_buffer);
}

void eg_mesh_asset_init(
    eg_mesh_asset_t *mesh,
    re_vertex_t *vertices,
    uint32_t vertex_count,
    uint32_t *indices,
    uint32_t index_count) {
  mesh->index_count = index_count;

  size_t vertex_buffer_size = sizeof(re_vertex_t) * vertex_count;
  size_t index_buffer_size = sizeof(uint32_t) * index_count;
  re_buffer_init(
      &mesh->vertex_buffer,
      &(re_buffer_options_t){
          .usage = RE_BUFFER_USAGE_VERTEX,
          .memory = RE_BUFFER_MEMORY_DEVICE,
          .size = vertex_buffer_size,
      });
  re_buffer_init(
      &mesh->index_buffer,
      &(re_buffer_options_t){
          .usage = RE_BUFFER_USAGE_INDEX,
          .memory = RE_BUFFER_MEMORY_DEVICE,
          .size = index_buffer_size,
      });

  re_buffer_t staging_buffer;
  re_buffer_init(
      &staging_buffer,
      &(re_buffer_options_t){
          .usage = RE_BUFFER_USAGE_TRANSFER,
          .memory = RE_BUFFER_MEMORY_HOST,
          .size = vertex_buffer_size > index_buffer_size ? vertex_buffer_size
                                                         : index_buffer_size,
      });

  void *memory;
  re_buffer_map_memory(&staging_buffer, &memory);

  memcpy(memory, vertices, vertex_buffer_size);
  re_buffer_transfer_to_buffer(
      &staging_buffer,
      &mesh->vertex_buffer,
      g_ctx.transient_command_pool,
      vertex_buffer_size);

  memcpy(memory, indices, index_buffer_size);
  re_buffer_transfer_to_buffer(
      &staging_buffer,
      &mesh->index_buffer,
      g_ctx.transient_command_pool,
      index_buffer_size);

  re_buffer_unmap_memory(&staging_buffer);

  re_buffer_destroy(&staging_buffer);
}

void eg_mesh_asset_draw(eg_mesh_asset_t *mesh, re_cmd_buffer_t *cmd_buffer) {
  re_cmd_bind_index_buffer(
      cmd_buffer, &mesh->index_buffer, 0, RE_INDEX_TYPE_UINT32);

  size_t offsets = 0;
  re_cmd_bind_vertex_buffers(cmd_buffer, 0, 1, &mesh->vertex_buffer, &offsets);

  re_cmd_draw_indexed(cmd_buffer, mesh->index_count, 1, 0, 0, 0);
}
