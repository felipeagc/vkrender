#pragma once

#include "cmd_info.h"
#include <gmath.h>
#include <renderer/buffer.h>

typedef struct re_pipeline_t re_pipeline_t;

typedef struct eg_pbr_model_uniform_t {
  mat4_t transform;
} eg_pbr_model_uniform_t;

typedef struct eg_pbr_model_t {
  eg_pbr_model_uniform_t uniform;
  re_buffer_t buffers[RE_MAX_FRAMES_IN_FLIGHT];
  void *mappings[RE_MAX_FRAMES_IN_FLIGHT];
  VkDescriptorSet descriptor_sets[RE_MAX_FRAMES_IN_FLIGHT];
} eg_pbr_model_t;

void eg_pbr_model_init(eg_pbr_model_t *model, mat4_t transform);

void eg_pbr_model_update_uniform(
    eg_pbr_model_t *model, const eg_cmd_info_t *cmd_info);

void eg_pbr_model_bind(
    eg_pbr_model_t *model,
    const eg_cmd_info_t *cmd_info,
    re_pipeline_t *pipeline,
    uint32_t set_index);

void eg_pbr_model_destroy(eg_pbr_model_t *model);
