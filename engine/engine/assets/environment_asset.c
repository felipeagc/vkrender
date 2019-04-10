#include "environment_asset.h"
#include "../engine.h"
#include "../filesystem.h"
#include "env_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void eg_environment_asset_init(
    eg_environment_asset_t *environment,
    const char *path,
    const char *brdf_lut_path) {
  eg_asset_init_named(&environment->asset, EG_ENVIRONMENT_ASSET_TYPE, path);

  uint8_t *skybox_data;
  uint32_t skybox_dim;

  uint8_t *irradiance_data;
  uint32_t irradiance_dim;

  uint8_t *radiance_data;
  uint32_t radiance_dim;
  uint32_t radiance_mip_levels;

  eg_file_t *env_file = eg_file_open_read(path);
  assert(env_file);
  size_t data_size = eg_file_size(env_file);
  assert(data_size > 0);
  unsigned char *data = calloc(1, data_size);
  assert(data);
  eg_file_read_bytes(env_file, data, data_size);
  eg_file_close(env_file);

  env_file_read(
      data,
      data_size,
      &skybox_data,
      &skybox_dim,
      &irradiance_data,
      &irradiance_dim,
      &radiance_data,
      &radiance_dim,
      &radiance_mip_levels);

  free(data);

  // Skybox
  re_image_init(
      &environment->skybox_cubemap,
      &(re_image_options_t){
          .data = skybox_data,
          .width = skybox_dim,
          .height = skybox_dim,
          .layer_count = 6,
          .mip_level_count = 1,
          .format = VK_FORMAT_R32G32B32A32_SFLOAT,
      });

  // Irradiance
  re_image_init(
      &environment->irradiance_cubemap,
      &(re_image_options_t){
          .data = irradiance_data,
          .width = irradiance_dim,
          .height = irradiance_dim,
          .layer_count = 6,
          .mip_level_count = 1,
          .format = VK_FORMAT_R32G32B32A32_SFLOAT,
      });

  // Radiance
  re_image_init(
      &environment->radiance_cubemap,
      &(re_image_options_t){
          .data = radiance_data,
          .width = radiance_dim,
          .height = radiance_dim,
          .layer_count = 6,
          .mip_level_count = radiance_mip_levels,
          .format = VK_FORMAT_R32G32B32A32_SFLOAT,
      });

  {
    eg_file_t *brdf_lut_file = eg_file_open_read(brdf_lut_path);
    assert(brdf_lut_file);
    size_t brdf_lut_raw_data_size = eg_file_size(brdf_lut_file);
    unsigned char *brdf_lut_raw_data = calloc(1, brdf_lut_raw_data_size);
    eg_file_read_bytes(
        brdf_lut_file, brdf_lut_raw_data, brdf_lut_raw_data_size);
    eg_file_close(brdf_lut_file);

    int width, height, channels;
    unsigned char *brdf_lut_data = stbi_load_from_memory(
        brdf_lut_raw_data,
        (int)brdf_lut_raw_data_size,
        &width,
        &height,
        &channels,
        4);

    free(brdf_lut_raw_data);

    re_image_init(
        &environment->brdf_lut,
        &(re_image_options_t){
            .data = brdf_lut_data,
            .width = (uint32_t)width,
            .height = (uint32_t)height,
        });

    free(brdf_lut_data);
  }

  free(skybox_data);
  free(irradiance_data);
  free(radiance_data);
}

void eg_environment_asset_destroy(eg_environment_asset_t *environment) {
  eg_asset_destroy(&environment->asset);

  re_image_destroy(&environment->skybox_cubemap);
  re_image_destroy(&environment->irradiance_cubemap);
  re_image_destroy(&environment->radiance_cubemap);

  re_image_destroy(&environment->brdf_lut);
}
