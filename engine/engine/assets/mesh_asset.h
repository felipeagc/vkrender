#pragma once

#include "asset_types.h"
#include <renderer/buffer.h>
#include <renderer/pipeline.h>

typedef struct eg_mesh_asset_options_t {
  uint32_t vertex_count;
  re_vertex_t *vertices;
  uint32_t index_count;
  uint32_t *indices;
} eg_mesh_asset_options_t;

typedef struct eg_mesh_asset_t {
  eg_asset_t asset;

  re_vertex_t *vertices;
  uint32_t vertex_count;
  uint32_t *indices;
  uint32_t index_count;

  re_buffer_t vertex_buffer;
  re_buffer_t index_buffer;
} eg_mesh_asset_t;

/*
 * Required asset functions
 */
void eg_mesh_asset_init(
    eg_mesh_asset_t *mesh, eg_mesh_asset_options_t *options);

void eg_mesh_asset_inspect(eg_mesh_asset_t *mesh, eg_inspector_t *inspector);

void eg_mesh_asset_destroy(eg_mesh_asset_t *mesh);

void eg_mesh_asset_serialize(
    eg_mesh_asset_t *mesh, eg_serializer_t *serializer);

void eg_mesh_asset_deserialize(
    eg_mesh_asset_t *mesh, eg_deserializer_t *deserializer);

/*
 * Specific functions
 */
void eg_mesh_asset_draw(eg_mesh_asset_t *mesh, re_cmd_buffer_t *cmd_buffer);
