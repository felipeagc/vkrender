#pragma once

#include <gmath.h>
#include <renderer/buffer.h>

typedef struct re_pipeline_t re_pipeline_t;
typedef struct eg_gltf_asset_t eg_gltf_asset_t;

typedef struct eg_gltf_comp_t {
  eg_gltf_asset_t *asset;
  mat4_t matrix;
} eg_gltf_comp_t;

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

void eg_gltf_comp_destroy(eg_gltf_comp_t *model);
