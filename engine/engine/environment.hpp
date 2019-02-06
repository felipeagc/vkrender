#pragma once

#include <renderer/buffer.hpp>
#include <renderer/common.hpp>
#include <renderer/resource_manager.hpp>
#include <vmath/vmath.h>

#define EG_MAX_POINT_LIGHTS 20

typedef struct eg_point_light_t {
  vec4_t pos;
  vec4_t color;
} eg_point_light_t;

typedef struct eg_environment_uniform_t {
  vec3_t sun_direction;
  float exposure;
  vec3_t sun_color;
  float sun_intensity;
  float radiance_mip_levels;
  uint32_t point_light_count;
  alignas(16) eg_point_light_t point_lights[EG_MAX_POINT_LIGHTS];
} eg_environment_uniform_t;

typedef struct eg_environment_t {
  eg_environment_uniform_t uniform;

  re_buffer_t uniform_buffers[RE_MAX_FRAMES_IN_FLIGHT];
  void *mappings[RE_MAX_FRAMES_IN_FLIGHT];
  re_resource_set_t resource_sets[RE_MAX_FRAMES_IN_FLIGHT];
} eg_environment_t;

void eg_environment_init(
    eg_environment_t *environment, struct eg_environment_asset_t *asset);

void eg_environment_update(
    eg_environment_t *environment, struct re_window_t *window);

void eg_environment_bind(
    eg_environment_t *environment,
    struct re_window_t *window,
    struct re_pipeline_t *pipeline,
    uint32_t set_index);

void eg_environment_draw_skybox(
    eg_environment_t *environment,
    struct re_window_t *window,
    struct re_pipeline_t *pipeline);

bool eg_environment_add_point_light(
    eg_environment_t *environment, const vec3_t pos, const vec3_t color);

void eg_environment_reset_point_lights(eg_environment_t *environment);

void eg_environment_destroy(eg_environment_t *environment);
