#pragma once

#include "pbr.hpp"

typedef struct eg_mesh_t {
  eg_pbr_model_t model;
  eg_pbr_model_t local_model;
  eg_pbr_material_t *material;
  struct eg_mesh_asset_t *asset;
} eg_mesh_t;

void eg_mesh_init(
    eg_mesh_t *mesh,
    struct eg_mesh_asset_t *asset,
    eg_pbr_material_t *material);

void eg_mesh_draw(
    eg_mesh_t *mesh,
    struct re_window_t *window,
    struct re_pipeline_t *pipeline);

void eg_mesh_destroy(eg_mesh_t *mesh);
