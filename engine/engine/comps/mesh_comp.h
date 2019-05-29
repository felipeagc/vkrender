#pragma once

#include "../assets/pbr_material_asset.h"

typedef struct eg_mesh_comp_t {
  struct {
    mat4_t model;
    mat4_t local_model;
  } uniform;
  eg_pbr_material_asset_t *material;
  struct eg_mesh_asset_t *asset;
} eg_mesh_comp_t;

void eg_mesh_comp_init(
    eg_mesh_comp_t *mesh,
    struct eg_mesh_asset_t *asset,
    eg_pbr_material_asset_t *material);

void eg_mesh_comp_draw(
    eg_mesh_comp_t *mesh,
    re_cmd_buffer_t *cmd_buffer,
    struct re_pipeline_t *pipeline);

void eg_mesh_comp_draw_no_mat(
    eg_mesh_comp_t *mesh,
    re_cmd_buffer_t *cmd_buffer,
    struct re_pipeline_t *pipeline);

void eg_mesh_comp_destroy(eg_mesh_comp_t *mesh);
