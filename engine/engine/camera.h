#pragma once

#include "cmd_info.h"
#include <gmath.h>
#include <renderer/buffer.h>
#include <renderer/common.h>

typedef struct re_window_t re_window_t;
typedef struct re_pipeline_t re_pipeline_t;

typedef struct eg_camera_uniform_t {
  mat4_t view;
  mat4_t proj;
  vec4_t pos;
} eg_camera_uniform_t;

typedef struct eg_camera_t {
  re_buffer_t uniform_buffers[RE_MAX_FRAMES_IN_FLIGHT];
  void *mappings[RE_MAX_FRAMES_IN_FLIGHT];
  VkDescriptorSet descriptor_sets[RE_MAX_FRAMES_IN_FLIGHT];

  float near_clip;
  float far_clip;

  float fov;
  eg_camera_uniform_t uniform;

  vec3_t position;
  quat_t rotation;
} eg_camera_t;

void eg_camera_init(eg_camera_t *camera);

void eg_camera_update(
    eg_camera_t *camera,
    const eg_cmd_info_t *cmd_info,
    float width,
    float height);

void eg_camera_bind(
    eg_camera_t *camera,
    const eg_cmd_info_t *cmd_info,
    struct re_pipeline_t *pipeline,
    uint32_t set_index);

void eg_camera_destroy(eg_camera_t *camera);
