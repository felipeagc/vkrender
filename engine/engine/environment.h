#pragma once

#include <fstd_util.h>
#include <gmath.h>
#include <renderer/buffer.h>

#define EG_MAX_POINT_LIGHTS 20

typedef struct re_pipeline_t re_pipeline_t;
typedef struct eg_image_asset_t eg_image_asset_t;

typedef enum eg_skybox_type_t {
  EG_SKYBOX_DEFAULT,
  EG_SKYBOX_IRRADIANCE,
} eg_skybox_type_t;

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
  ALIGNAS(16) eg_point_light_t point_lights[EG_MAX_POINT_LIGHTS];
} eg_environment_uniform_t;

typedef struct eg_environment_t {
  eg_image_asset_t *skybox;
  eg_image_asset_t *irradiance;
  eg_image_asset_t *radiance;
  eg_image_asset_t *brdf;

  eg_skybox_type_t skybox_type;
  eg_environment_uniform_t uniform;
} eg_environment_t;

void eg_environment_init(
    eg_environment_t *environment,
    eg_image_asset_t *skybox,
    eg_image_asset_t *irradiance,
    eg_image_asset_t *radiance,
    eg_image_asset_t *brdf);

void eg_environment_bind(
    eg_environment_t *environment,
    re_cmd_buffer_t *cmd_buffer,
    re_pipeline_t *pipeline,
    uint32_t set_index);

void eg_environment_draw_skybox(
    eg_environment_t *environment,
    re_cmd_buffer_t *cmd_buffer,
    re_pipeline_t *pipeline);

void eg_environment_set_skybox(
    eg_environment_t *environment, eg_skybox_type_t type);

bool eg_environment_add_point_light(
    eg_environment_t *environment, const vec3_t pos, const vec4_t color);

void eg_environment_reset_point_lights(eg_environment_t *environment);

void eg_environment_destroy(eg_environment_t *environment);
