#pragma once

#include <gmath.h>
#include <renderer/buffer.h>

typedef struct re_pipeline_t re_pipeline_t;

typedef struct eg_camera_uniform_t {
  mat4_t view;
  mat4_t proj;
  vec4_t pos;
} eg_camera_uniform_t;

typedef struct eg_camera_t {
  float near_clip;
  float far_clip;

  float fov;
  eg_camera_uniform_t uniform;

  vec3_t position;
  quat_t rotation;
} eg_camera_t;

void eg_camera_init(eg_camera_t *camera);

void eg_camera_update(eg_camera_t *camera, re_cmd_buffer_t *cmd_buffer);

void eg_camera_bind(
    eg_camera_t *camera,
    re_cmd_buffer_t *cmd_buffer,
    re_pipeline_t *pipeline,
    uint32_t set_index);

void eg_camera_destroy(eg_camera_t *camera);

vec3_t eg_camera_ndc_to_world(eg_camera_t *camera, vec3_t ndc);

vec3_t eg_camera_world_to_ndc(eg_camera_t *camera, vec3_t world);
