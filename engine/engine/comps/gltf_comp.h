#pragma once

#include "../cmd_info.h"
#include <gmath.h>
#include <renderer/buffer.h>

typedef struct re_pipeline_t re_pipeline_t;
typedef struct eg_gltf_asset_t eg_gltf_asset_t;

typedef struct eg_gltf_comp_t {
  eg_gltf_asset_t *asset;

  re_buffer_t uniform_buffers[RE_MAX_FRAMES_IN_FLIGHT];
  void *mappings[RE_MAX_FRAMES_IN_FLIGHT];
  VkDescriptorSet descriptor_sets[RE_MAX_FRAMES_IN_FLIGHT];

  struct {
    mat4_t matrix;
  } ubo;
} eg_gltf_comp_t;

void eg_gltf_comp_init(
    eg_gltf_comp_t *model, eg_gltf_asset_t *asset);

void eg_gltf_comp_draw(
    eg_gltf_comp_t *model,
    const eg_cmd_info_t *cmd_info,
    re_pipeline_t *pipeline,
    mat4_t transform);

void eg_gltf_comp_draw_no_mat(
    eg_gltf_comp_t *model,
    const eg_cmd_info_t *cmd_info,
    re_pipeline_t *pipeline,
    mat4_t transform);

void eg_gltf_comp_destroy(eg_gltf_comp_t *model);
