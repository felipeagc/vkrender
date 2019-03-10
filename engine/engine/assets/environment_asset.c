#include "environment_asset.h"
#include "env_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void eg_environment_asset_init(
    eg_environment_asset_t *environment,
    const char *path,
    const char *brdf_lut_path) {
  eg_asset_init_named(&environment->asset, EG_ENVIRONMENT_ASSET_TYPE, path);

  env_file_read_options_t options = {
      .path = path,
  };

  env_file_read(&options);

  // Skybox
  re_cubemap_init_from_data(
      &environment->skybox_cubemap,
      options.skybox_layers,
      6,
      1,
      options.skybox_dim,
      options.skybox_dim);

  // Irradiance
  re_cubemap_init_from_data(
      &environment->irradiance_cubemap,
      options.irradiance_layers,
      6,
      1,
      options.irradiance_dim,
      options.irradiance_dim);

  // Radiance
  re_cubemap_init_from_data(
      &environment->radiance_cubemap,
      options.radiance_layers[0],
      6,
      options.radiance_mip_count,
      options.base_radiance_dim,
      options.base_radiance_dim);

  re_texture_init_from_path(&environment->brdf_lut, brdf_lut_path);

  for (uint32_t layer = 0; layer < 6; layer++) {
    free(options.skybox_layers[layer]);
    free(options.irradiance_layers[layer]);
    for (uint32_t level = 0; level < options.radiance_mip_count; level++) {
      free(options.radiance_layers[level][layer]);
    }
  }
}

void eg_environment_asset_destroy(eg_environment_asset_t *environment) {
  eg_asset_destroy(&environment->asset);

  re_cubemap_destroy(&environment->skybox_cubemap);
  re_cubemap_destroy(&environment->irradiance_cubemap);
  re_cubemap_destroy(&environment->radiance_cubemap);

  re_texture_destroy(&environment->brdf_lut);
}
