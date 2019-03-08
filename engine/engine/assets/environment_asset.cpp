#include "environment_asset.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void eg_environment_asset_init(
    eg_environment_asset_t *environment,
    const char *env_dir,
    uint32_t radiance_mip_map_count,
    const char *brdf_lut_path) {
  eg_asset_init_named(&environment->asset, EG_ENVIRONMENT_ASSET_TYPE, env_dir);

  const char *skybox_prefix = "skybox_side_";
  const char *skybox_suffix = ".hdr";
  char *skybox_paths[6];
  for (uint32_t i = 0; i < 6; i++) {
    skybox_paths[i] = (char *)malloc(
        strlen(env_dir) + 1 + strlen(skybox_prefix) + 1 +
        strlen(skybox_suffix) + 1);
    sprintf(
        skybox_paths[i], "%s/%s%d%s", env_dir, skybox_prefix, i, skybox_suffix);
  }

  const char *irradiance_prefix = "irradiance_side_";
  const char *irradiance_suffix = ".hdr";
  char *irradiance_paths[6];
  for (uint32_t i = 0; i < 6; i++) {
    irradiance_paths[i] = (char *)malloc(
        strlen(env_dir) + 1 + strlen(irradiance_prefix) + 1 +
        strlen(irradiance_suffix) + 1);
    sprintf(
        irradiance_paths[i],
        "%s/%s%d%s",
        env_dir,
        irradiance_prefix,
        i,
        irradiance_suffix);
  }

  const char *radiance_prefix = "radiance_side_";
  const char *radiance_midfix = "_mip_";
  const char *radiance_suffix = ".hdr";
  char **radiance_paths[6];
  for (uint32_t i = 0; i < 6; i++) {
    radiance_paths[i] =
        (char **)malloc(sizeof(char *) * radiance_mip_map_count);
    for (uint32_t level = 0; level < radiance_mip_map_count; level++) {
      radiance_paths[i][level] = (char *)malloc(
          strlen(env_dir) + 1 + strlen(radiance_prefix) + 1 +
          strlen(radiance_midfix) + 1 + strlen(radiance_suffix) + 1);
      sprintf(
          radiance_paths[i][level],
          "%s/%s%d%s%d%s",
          env_dir,
          radiance_prefix,
          i,
          radiance_midfix,
          level,
          radiance_suffix);
    }
  }

  re_cubemap_init_from_hdr_sides(&environment->skybox_cubemap, skybox_paths);
  re_cubemap_init_from_hdr_sides(
      &environment->irradiance_cubemap, irradiance_paths);
  re_cubemap_init_from_hdr_sides_with_mip_maps(
      &environment->radiance_cubemap, radiance_paths, radiance_mip_map_count);

  re_texture_init_from_path(&environment->brdf_lut, brdf_lut_path);

  for (uint32_t i = 0; i < 6; i++) {
    free(skybox_paths[i]);
    free(irradiance_paths[i]);
    for (uint32_t level = 0; level < radiance_mip_map_count; level++) {
      free(radiance_paths[i][level]);
    }
    free(radiance_paths[i]);
  }
}

void eg_environment_asset_destroy(eg_environment_asset_t *environment) {
  eg_asset_destroy(&environment->asset);

  re_cubemap_destroy(&environment->skybox_cubemap);
  re_cubemap_destroy(&environment->irradiance_cubemap);
  re_cubemap_destroy(&environment->radiance_cubemap);

  re_texture_destroy(&environment->brdf_lut);
}
