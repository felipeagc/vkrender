#pragma once

#include <gmath.h>

typedef struct re_cmd_buffer_t re_cmd_buffer_t;
typedef struct re_pipeline_t re_pipeline_t;
typedef struct eg_pbr_material_asset_t eg_pbr_material_asset_t;
typedef struct eg_mesh_asset_t eg_mesh_asset_t;
typedef struct eg_inspector_t eg_inspector_t;

typedef struct eg_mesh_comp_t {
  eg_pbr_material_asset_t *material;
  eg_mesh_asset_t *asset;
} eg_mesh_comp_t;

/*
 * Required component functions
 */
void eg_mesh_comp_default(eg_mesh_comp_t *mesh);

void eg_mesh_comp_inspect(eg_mesh_comp_t *mesh, eg_inspector_t *inspector);

void eg_mesh_comp_destroy(eg_mesh_comp_t *mesh);

/*
 * Specific functions
 */
void eg_mesh_comp_init(
    eg_mesh_comp_t *mesh,
    eg_mesh_asset_t *asset,
    eg_pbr_material_asset_t *material);

void eg_mesh_comp_draw(
    eg_mesh_comp_t *mesh,
    re_cmd_buffer_t *cmd_buffer,
    re_pipeline_t *pipeline,
    mat4_t transform);

void eg_mesh_comp_draw_no_mat(
    eg_mesh_comp_t *mesh,
    re_cmd_buffer_t *cmd_buffer,
    re_pipeline_t *pipeline,
    mat4_t transform);

