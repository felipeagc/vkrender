#include "mesh_asset.h"

#include "../asset_manager.h"
#include "../deserializer.h"
#include "../serializer.h"
#include <renderer/context.h>
#include <renderer/window.h>
#include <string.h>

void eg_mesh_asset_init(
    eg_mesh_asset_t *mesh, eg_mesh_asset_options_t *options) {
  mesh->vertex_count = options->vertex_count;
  mesh->index_count  = options->index_count;
  mesh->vertices     = malloc(sizeof(*mesh->vertices) * mesh->vertex_count);
  mesh->indices      = malloc(sizeof(*mesh->indices) * mesh->index_count);

  memcpy(
      mesh->vertices,
      options->vertices,
      sizeof(*mesh->vertices) * mesh->vertex_count);
  memcpy(
      mesh->indices,
      options->indices,
      sizeof(*mesh->indices) * mesh->index_count);

  size_t vertex_buffer_size = sizeof(re_vertex_t) * options->vertex_count;
  size_t index_buffer_size  = sizeof(uint32_t) * options->index_count;

  re_buffer_init(
      &mesh->vertex_buffer,
      &(re_buffer_options_t){
          .usage  = RE_BUFFER_USAGE_VERTEX,
          .memory = RE_BUFFER_MEMORY_DEVICE,
          .size   = vertex_buffer_size,
      });
  re_buffer_init(
      &mesh->index_buffer,
      &(re_buffer_options_t){
          .usage  = RE_BUFFER_USAGE_INDEX,
          .memory = RE_BUFFER_MEMORY_DEVICE,
          .size   = index_buffer_size,
      });

  re_buffer_t staging_buffer;
  re_buffer_init(
      &staging_buffer,
      &(re_buffer_options_t){
          .usage  = RE_BUFFER_USAGE_TRANSFER,
          .memory = RE_BUFFER_MEMORY_HOST,
          .size   = vertex_buffer_size > index_buffer_size ? vertex_buffer_size
                                                         : index_buffer_size,
      });

  void *memory;
  re_buffer_map_memory(&staging_buffer, &memory);

  memcpy(memory, options->vertices, vertex_buffer_size);
  re_buffer_transfer_to_buffer(
      &staging_buffer,
      &mesh->vertex_buffer,
      g_ctx.transient_command_pool,
      vertex_buffer_size);

  memcpy(memory, options->indices, index_buffer_size);
  re_buffer_transfer_to_buffer(
      &staging_buffer,
      &mesh->index_buffer,
      g_ctx.transient_command_pool,
      index_buffer_size);

  re_buffer_unmap_memory(&staging_buffer);

  re_buffer_destroy(&staging_buffer);
}

void eg_mesh_asset_inspect(eg_mesh_asset_t *mesh, eg_inspector_t *inspector) {}

void eg_mesh_asset_destroy(eg_mesh_asset_t *mesh) {
  re_buffer_destroy(&mesh->vertex_buffer);
  re_buffer_destroy(&mesh->index_buffer);

  free(mesh->vertices);
  free(mesh->indices);
}

enum {
  PROP_VERTICES,
  PROP_INDICES,
  PROP_MAX,
};

void eg_mesh_asset_serialize(
    eg_mesh_asset_t *mesh, eg_serializer_t *serializer) {
  eg_serializer_append_u32(serializer, PROP_MAX);

  // Vertices
  eg_serializer_append_u32(serializer, PROP_VERTICES);
  eg_serializer_append_u32(serializer, mesh->vertex_count);
  eg_serializer_append(
      serializer, mesh->vertices, sizeof(*mesh->vertices) * mesh->vertex_count);

  // Indices
  eg_serializer_append_u32(serializer, PROP_INDICES);
  eg_serializer_append_u32(serializer, mesh->index_count);
  eg_serializer_append(
      serializer, mesh->indices, sizeof(*mesh->indices) * mesh->index_count);
}

void eg_mesh_asset_deserialize(
    eg_mesh_asset_t *mesh, eg_deserializer_t *deserializer) {
  uint32_t prop_count = eg_deserializer_read_u32(deserializer);

  eg_mesh_asset_options_t options = {0};

  for (uint32_t i = 0; i < prop_count; i++) {
    uint32_t prop = eg_deserializer_read_u32(deserializer);

    switch (prop) {
    case PROP_VERTICES: {
      options.vertex_count = eg_deserializer_read_u32(deserializer);
      options.vertices     = malloc(sizeof(re_vertex_t) * options.vertex_count);
      eg_deserializer_read(
          deserializer,
          options.vertices,
          sizeof(re_vertex_t) * options.vertex_count);
      break;
    }
    case PROP_INDICES: {
      options.index_count = eg_deserializer_read_u32(deserializer);
      options.indices     = malloc(sizeof(uint32_t) * options.index_count);
      eg_deserializer_read(
          deserializer,
          options.indices,
          sizeof(uint32_t) * options.index_count);
      break;
    }
    default: break;
    }
  }

  eg_mesh_asset_init(mesh, &options);

  free(options.vertices);
  free(options.indices);
}

void eg_mesh_asset_draw(eg_mesh_asset_t *mesh, re_cmd_buffer_t *cmd_buffer) {
  re_cmd_bind_index_buffer(
      cmd_buffer, &mesh->index_buffer, 0, RE_INDEX_TYPE_UINT32);

  size_t offsets = 0;
  re_cmd_bind_vertex_buffers(cmd_buffer, 0, 1, &mesh->vertex_buffer, &offsets);

  re_cmd_draw_indexed(cmd_buffer, mesh->index_count, 1, 0, 0, 0);
}
