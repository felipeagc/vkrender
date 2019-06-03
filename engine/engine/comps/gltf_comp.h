#pragma once

#include <gmath.h>

typedef struct re_cmd_buffer_t re_cmd_buffer_t;
typedef struct re_pipeline_t re_pipeline_t;
typedef struct eg_gltf_asset_t eg_gltf_asset_t;
typedef struct eg_inspector_t eg_inspector_t;

typedef struct eg_gltf_comp_t {
  eg_gltf_asset_t *asset;
} eg_gltf_comp_t;

/*
 * Required component functions
 */
void eg_gltf_comp_default(eg_gltf_comp_t *model);

void eg_gltf_comp_inspect(eg_gltf_comp_t *model, eg_inspector_t *inspector);

void eg_gltf_comp_destroy(eg_gltf_comp_t *model);

/*
 * Specific functions
 */
void eg_gltf_comp_init(eg_gltf_comp_t *model, eg_gltf_asset_t *asset);

void eg_gltf_comp_draw(
    eg_gltf_comp_t *model,
    re_cmd_buffer_t *cmd_buffer,
    re_pipeline_t *pipeline,
    mat4_t transform);

void eg_gltf_comp_draw_no_mat(
    eg_gltf_comp_t *model,
    re_cmd_buffer_t *cmd_buffer,
    re_pipeline_t *pipeline,
    mat4_t transform);

