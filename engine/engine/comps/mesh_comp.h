#pragma once

#include "../assets/pbr_material_asset.h"
#include "../cmd_info.h"
#include "../pbr.h"

typedef struct eg_mesh_comp_t {
  eg_pbr_model_t model;
  eg_pbr_model_t local_model;
  eg_pbr_material_asset_t *material;
  struct eg_mesh_asset_t *asset;
} eg_mesh_comp_t;

void eg_mesh_comp_init(
    eg_mesh_comp_t *mesh,
    struct eg_mesh_asset_t *asset,
    eg_pbr_material_asset_t *material);

void eg_mesh_comp_draw(
    eg_mesh_comp_t *mesh,
    const eg_cmd_info_t *cmd_info,
    struct re_pipeline_t *pipeline);

void eg_mesh_comp_destroy(eg_mesh_comp_t *mesh);