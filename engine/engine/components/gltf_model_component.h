#pragma once

#include <gmath.h>
#include <renderer/buffer.h>
#include <renderer/common.h>

typedef struct re_window_t re_window_t;
typedef struct re_pipeline_t re_pipeline_t;
typedef struct eg_gltf_model_asset_t eg_gltf_model_asset_t;

typedef struct eg_gltf_model_component {
  eg_gltf_model_asset_t *asset;

  re_buffer_t uniform_buffers[RE_MAX_FRAMES_IN_FLIGHT];
  void *mappings[RE_MAX_FRAMES_IN_FLIGHT];
  VkDescriptorSet descriptor_sets[RE_MAX_FRAMES_IN_FLIGHT];

  struct {
    mat4_t matrix;
  } ubo;
} eg_gltf_model_component_t;

void eg_gltf_model_component_init(
    eg_gltf_model_component_t *model, eg_gltf_model_asset_t *asset);

void eg_gltf_model_component_draw(
    eg_gltf_model_component_t *model,
    re_window_t *window,
    re_pipeline_t *pipeline,
    mat4_t transform);

void eg_gltf_model_component_draw_picking(
    eg_gltf_model_component_t *model,
    re_window_t *window,
    VkCommandBuffer command_buffer,
    re_pipeline_t *pipeline,
    mat4_t transform);

void eg_gltf_model_component_destroy(eg_gltf_model_component_t *model);
