#pragma once

#include "asset_types.h"
#include <renderer/image.h>
#include <stdint.h>

typedef struct eg_environment_asset_t {
  eg_asset_t asset;

  re_image_t skybox_cubemap;
  re_image_t irradiance_cubemap;
  re_image_t radiance_cubemap;
  re_image_t brdf_lut;
} eg_environment_asset_t;

void eg_environment_asset_init(
    eg_environment_asset_t *environment,
    const char *path,
    const char *brdf_lut_path);

void eg_environment_asset_destroy(eg_environment_asset_t *environment);
