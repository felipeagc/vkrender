#include "environment_asset.hpp"

void eg_environment_asset_init(
    eg_environment_asset_t *environment,
    uint32_t width,
    uint32_t height,
    const char *skybox_path,
    const char *irradiance_path,
    uint32_t radiance_paths_count,
    const char **radiance_paths,
    const char *brdf_lut_path) {
  eg_asset_init_named(
      &environment->asset,
      (eg_asset_destructor_t)eg_environment_asset_destroy,
      skybox_path);

  re_cubemap_init_from_hdr_equirec(
      &environment->skybox_cubemap, skybox_path, width, height);
  re_cubemap_init_from_hdr_equirec(
      &environment->irradiance_cubemap, irradiance_path, width, height);
  re_cubemap_init_from_hdr_equirec_mipmaps(
      &environment->radiance_cubemap,
      radiance_paths,
      radiance_paths_count,
      width,
      height);

  re_texture_init_from_path(&environment->brdf_lut, brdf_lut_path);
}

void eg_environment_asset_destroy(eg_environment_asset_t *environment) {
  eg_asset_destroy(&environment->asset);

  re_cubemap_destroy(&environment->skybox_cubemap);
  re_cubemap_destroy(&environment->irradiance_cubemap);
  re_cubemap_destroy(&environment->radiance_cubemap);

  re_texture_destroy(&environment->brdf_lut);
}
