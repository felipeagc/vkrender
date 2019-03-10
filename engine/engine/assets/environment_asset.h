#pragma once

#include "asset_types.h"
#include <renderer/cubemap.h>
#include <renderer/texture.h>
#include <stdint.h>

typedef struct eg_environment_asset_t {
  eg_asset_t asset;

  re_cubemap_t skybox_cubemap;
  re_cubemap_t irradiance_cubemap;
  re_cubemap_t radiance_cubemap;
  re_texture_t brdf_lut;
} eg_environment_asset_t;

void eg_environment_asset_init(
    eg_environment_asset_t *environment,
    const char* path,
    const char *brdf_lut_path);

void eg_environment_asset_destroy(eg_environment_asset_t *environment);
