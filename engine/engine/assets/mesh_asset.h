#pragma once

#include "../cmd_info.h"
#include "../pbr.h"
#include "asset_types.h"
#include <renderer/buffer.h>
#include <renderer/pipeline.h>

typedef struct eg_mesh_asset_t {
  eg_asset_t asset;

  re_buffer_t vertex_buffer;
  re_buffer_t index_buffer;
  uint32_t index_count;
} eg_mesh_asset_t;

void eg_mesh_asset_init(
    eg_mesh_asset_t *mesh,
    re_vertex_t *vertices,
    uint32_t vertex_count,
    uint32_t *indices,
    uint32_t index_count);

void eg_mesh_asset_draw(eg_mesh_asset_t *mesh, const eg_cmd_info_t *cmd_info);

void eg_mesh_asset_destroy(eg_mesh_asset_t *mesh);
