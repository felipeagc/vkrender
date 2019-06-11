#include "environment.h"

#include "assets/image_asset.h"
#include "pipelines.h"
#include <fstd_util.h>
#include <renderer/context.h>
#include <renderer/pipeline.h>
#include <renderer/util.h>
#include <renderer/window.h>
#include <string.h>

// For skybox pipeline
#define ENVIRONMENT_SET_INDEX 1

void eg_environment_init(
    eg_environment_t *environment,
    eg_image_asset_t *skybox,
    eg_image_asset_t *irradiance,
    eg_image_asset_t *radiance,
    eg_image_asset_t *brdf) {
  environment->skybox     = skybox;
  environment->irradiance = irradiance;
  environment->radiance   = radiance;
  environment->brdf       = brdf;

  environment->skybox_type = EG_SKYBOX_DEFAULT;

  environment->uniform.sun_direction       = (vec3_t){0.0, -1.0, 0.0};
  environment->uniform.exposure            = 8.0f;
  environment->uniform.sun_color           = (vec3_t){1.0f, 1.0f, 1.0f};
  environment->uniform.sun_intensity       = 1.0f;
  environment->uniform.radiance_mip_levels = 1.0f;
  environment->uniform.point_light_count   = 0;

  environment->uniform.radiance_mip_levels =
      (float)environment->radiance->image.mip_level_count;
}

void eg_environment_bind(
    eg_environment_t *environment,
    re_cmd_buffer_t *cmd_buffer,
    struct re_pipeline_t *pipeline,
    uint32_t set) {
  re_cmd_bind_pipeline(cmd_buffer, pipeline);

  void *mapping =
      re_cmd_bind_uniform(cmd_buffer, set, 0, sizeof(environment->uniform));
  memcpy(mapping, &environment->uniform, sizeof(environment->uniform));

  re_cmd_bind_image(cmd_buffer, set, 1, &environment->irradiance->image);
  re_cmd_bind_image(cmd_buffer, set, 2, &environment->radiance->image);
  re_cmd_bind_image(cmd_buffer, set, 3, &environment->brdf->image);

  re_cmd_bind_descriptor_set(cmd_buffer, pipeline, set);
}

void eg_environment_draw_skybox(
    eg_environment_t *environment,
    re_cmd_buffer_t *cmd_buffer,
    struct re_pipeline_t *pipeline) {
  re_cmd_bind_pipeline(cmd_buffer, pipeline);

  void *mapping =
      re_cmd_bind_uniform(cmd_buffer, 1, 0, sizeof(environment->uniform));
  memcpy(mapping, &environment->uniform, sizeof(environment->uniform));

  switch (environment->skybox_type) {
  case EG_SKYBOX_DEFAULT:
    re_cmd_bind_image(cmd_buffer, 1, 1, &environment->skybox->image);
    break;
  case EG_SKYBOX_IRRADIANCE:
    re_cmd_bind_image(cmd_buffer, 1, 1, &environment->irradiance->image);
    break;
  }

  re_cmd_bind_descriptor_set(cmd_buffer, pipeline, 1);

  re_cmd_draw(cmd_buffer, 36, 1, 0, 0);
}

bool eg_environment_add_point_light(
    eg_environment_t *environment, const vec3_t pos, const vec4_t color) {
  if (environment->uniform.point_light_count + 1 == EG_MAX_POINT_LIGHTS) {
    return false;
  }

  environment->uniform.point_lights[environment->uniform.point_light_count] =
      (eg_point_light_t){(vec4_t){pos.x, pos.y, pos.z, 1.0}, color};
  environment->uniform.point_light_count++;
  return true;
}

void eg_environment_reset_point_lights(eg_environment_t *environment) {
  environment->uniform.point_light_count = 0;
}

void eg_environment_destroy(eg_environment_t *environment) {}
