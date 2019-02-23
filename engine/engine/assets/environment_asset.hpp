#pragma once

#include "asset_types.hpp"
#include <renderer/cubemap.hpp>
#include <renderer/texture.hpp>
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
    uint32_t width,
    uint32_t height,
    const char *skybox_path,
    const char *irradiance_path,
    uint32_t radiance_paths_count,
    const char **radiance_paths,
    const char *brdf_lut_path);

void eg_environment_asset_destroy(eg_environment_asset_t *environment);
