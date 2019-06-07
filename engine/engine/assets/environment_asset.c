#include "environment_asset.h"
#include "../filesystem.h"
#include "../util/tinyktx.h"
#include <renderer/context.h>
#include <stb_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline void
upload_ktx(re_image_t *image, re_cmd_pool_t pool, ktx_data_t *ktx_data) {
  for (uint32_t mip_level = 0; mip_level < ktx_data->mipmap_level_count;
       mip_level++) {
    ktx_mip_level_t *mip_ptr = &ktx_data->mip_levels[mip_level];
    for (uint32_t face = 0; face < ktx_data->face_count; face++) {
      ktx_face_t *face_ptr = &mip_ptr->array_elements[0].faces[face];

      uint32_t mip_width = ktx_data->pixel_width / (1 << mip_level);
      uint32_t mip_height = ktx_data->pixel_height / (1 << mip_level);

      re_image_upload(
          image,
          pool,
          face_ptr->slices[0].data,
          mip_width,
          mip_height,
          mip_level,
          face);
    }
  }
}

void eg_environment_asset_inspect(
    eg_environment_asset_t *environment, eg_inspector_t *inspector) {}

void eg_environment_asset_destroy(eg_environment_asset_t *environment) {
  re_image_destroy(&environment->skybox_cubemap);
  re_image_destroy(&environment->irradiance_cubemap);
  re_image_destroy(&environment->radiance_cubemap);

  re_image_destroy(&environment->brdf_lut);
}

void eg_environment_asset_init(
    eg_environment_asset_t *environment,
    const char *skybox_path,
    const char *irradiance_path,
    const char *radiance_path,
    const char *brdf_lut_path) {

  ktx_data_t ktx_data;
  ktx_result_t ktx_result;

  // Skybox
  {
    eg_file_t *file = eg_file_open_read(skybox_path);
    assert(file);
    size_t raw_data_size = eg_file_size(file);
    uint8_t *raw_data = calloc(1, raw_data_size);
    eg_file_read_bytes(file, raw_data, raw_data_size);
    eg_file_close(file);

    ktx_result = ktx_read(raw_data, raw_data_size, &ktx_data);
    assert(KTX_SUCCESS == ktx_result);
    assert(KTX_RGBA16F == ktx_data.internal_format);
    assert(6 == ktx_data.face_count);

    re_image_init(
        &environment->skybox_cubemap,
        &(re_image_options_t){
            .width = ktx_data.pixel_width,
            .height = ktx_data.pixel_height,
            .layer_count = ktx_data.face_count,
            .mip_level_count = ktx_data.mipmap_level_count,
            .format = VK_FORMAT_R16G16B16A16_SFLOAT,
            .anisotropy = true,
        });

    upload_ktx(
        &environment->skybox_cubemap, g_ctx.transient_command_pool, &ktx_data);

    ktx_data_destroy(&ktx_data);
  }

  // Irradiance
  {
    eg_file_t *file = eg_file_open_read(irradiance_path);
    assert(file);
    size_t raw_data_size = eg_file_size(file);
    uint8_t *raw_data = calloc(1, raw_data_size);
    eg_file_read_bytes(file, raw_data, raw_data_size);
    eg_file_close(file);

    ktx_result = ktx_read(raw_data, raw_data_size, &ktx_data);
    assert(KTX_SUCCESS == ktx_result);
    assert(KTX_RGBA16F == ktx_data.internal_format);
    assert(1 == ktx_data.mipmap_level_count);
    assert(6 == ktx_data.face_count);

    re_image_init(
        &environment->irradiance_cubemap,
        &(re_image_options_t){
            .width = ktx_data.pixel_width,
            .height = ktx_data.pixel_height,
            .layer_count = ktx_data.face_count,
            .mip_level_count = ktx_data.mipmap_level_count,
            .format = VK_FORMAT_R16G16B16A16_SFLOAT,
        });

    upload_ktx(
        &environment->irradiance_cubemap,
        g_ctx.transient_command_pool,
        &ktx_data);

    ktx_data_destroy(&ktx_data);
  }

  // Radiance
  {
    eg_file_t *file = eg_file_open_read(radiance_path);
    assert(file);
    size_t raw_data_size = eg_file_size(file);
    uint8_t *raw_data = calloc(1, raw_data_size);
    eg_file_read_bytes(file, raw_data, raw_data_size);
    eg_file_close(file);

    ktx_result = ktx_read(raw_data, raw_data_size, &ktx_data);
    assert(KTX_SUCCESS == ktx_result);
    assert(KTX_RGBA16F == ktx_data.internal_format);
    assert(6 == ktx_data.face_count);

    re_image_init(
        &environment->radiance_cubemap,
        &(re_image_options_t){
            .width = ktx_data.pixel_width,
            .height = ktx_data.pixel_height,
            .layer_count = ktx_data.face_count,
            .mip_level_count = ktx_data.mipmap_level_count,
            .format = VK_FORMAT_R16G16B16A16_SFLOAT,
        });

    upload_ktx(
        &environment->radiance_cubemap,
        g_ctx.transient_command_pool,
        &ktx_data);

    ktx_data_destroy(&ktx_data);
  }

  // BRDF LuT
  {
    eg_file_t *file = eg_file_open_read(brdf_lut_path);
    assert(file);
    size_t raw_data_size = eg_file_size(file);
    uint8_t *raw_data = calloc(1, raw_data_size);
    eg_file_read_bytes(file, raw_data, raw_data_size);
    eg_file_close(file);

    int width, height, channels;
    uint8_t *data = stbi_load_from_memory(
        raw_data, (int)raw_data_size, &width, &height, &channels, 4);

    free(raw_data);

    re_image_init(
        &environment->brdf_lut,
        &(re_image_options_t){
            .width = (uint32_t)width,
            .height = (uint32_t)height,
        });

    re_image_upload(
        &environment->brdf_lut,
        g_ctx.transient_command_pool,
        data,
        (uint32_t)width,
        (uint32_t)height,
        0,
        0);

    free(data);
  }
}
