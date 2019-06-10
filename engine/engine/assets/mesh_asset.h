#pragma once

#include "asset_types.h"
#include <renderer/buffer.h>
#include <renderer/pipeline.h>

typedef struct eg_mesh_asset_t {
  eg_asset_t asset;

  re_buffer_t vertex_buffer;
  re_buffer_t index_buffer;
  uint32_t index_count;
} eg_mesh_asset_t;

/*
 * Required asset functions
 */
void eg_mesh_asset_inspect(eg_mesh_asset_t *mesh, eg_inspector_t *inspector);

void eg_mesh_asset_destroy(eg_mesh_asset_t *mesh);

/*
 * Specific functions
 */
void eg_mesh_asset_init(
    eg_mesh_asset_t *mesh,
    re_vertex_t *vertices,
    uint32_t vertex_count,
    uint32_t *indices,
    uint32_t index_count);

void eg_mesh_asset_draw(eg_mesh_asset_t *mesh, re_cmd_buffer_t *cmd_buffer);
