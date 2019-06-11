#include "image_asset.h"

#include "../filesystem.h"
#include "../imgui.h"
#include "../util.h"
#include "../util/tinyktx.h"
#include <assert.h>
#include <renderer/context.h>
#include <stb_image.h>
#include <stdbool.h>
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

      uint32_t mip_width  = ktx_data->pixel_width / (1 << mip_level);
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

// Returns the file extension without the dot
static inline void get_ext(const char *path, char *out_ext) {
  uint32_t dot_pos = 0;

  for (uint32_t i = 0; i < strlen(path); i++) {
    if (path[i] == '.') {
      dot_pos = i + 1;
    }
  }

  strncpy(out_ext, &path[dot_pos], strlen(path) - dot_pos + 1);
}

void eg_image_asset_inspect(
    eg_image_asset_t *image, eg_inspector_t *inspector) {
  igText("Dimensions: %ux%u", image->image.width, image->image.height);
  igText("Layers: %u", image->image.layer_count);
  igText("Mip levels: %u", image->image.mip_level_count);

  const float img_size = 128.0f;

  // TODO: display cubemaps
  if (image->image.layer_count == 1) {
    igImage(
        &image->image,
        (ImVec2){img_size, img_size},
        (ImVec2){0.0f, 0.0f},
        (ImVec2){1.0f, 1.0f},
        (ImVec4){1.0f, 1.0f, 1.0f, 1.0f},
        (ImVec4){0.0f, 0.0f, 0.0f, 0.0f});
  }
}

void eg_image_asset_destroy(eg_image_asset_t *image) {
  re_image_destroy(&image->image);
}

void eg_image_asset_init(eg_image_asset_t *image, const char *path) {
  char ext[128] = "";
  get_ext(path, ext);

  if (strcmp(ext, "ktx") == 0) {
    ktx_data_t ktx_data;
    ktx_result_t ktx_result;

    eg_file_t *file = eg_file_open_read(path);
    assert(file);
    size_t raw_data_size = eg_file_size(file);
    uint8_t *raw_data    = calloc(1, raw_data_size);
    eg_file_read_bytes(file, raw_data, raw_data_size);
    eg_file_close(file);

    ktx_result = ktx_read(raw_data, raw_data_size, &ktx_data);
    assert(KTX_SUCCESS == ktx_result);

    VkFormat format;

    switch (ktx_data.internal_format) {
    case KTX_RGBA16F: {
      format = VK_FORMAT_R16G16B16A16_SFLOAT;
      break;
    }
    default: {
      EG_LOG_FATAL(
          "Unsupported KTX internal format: %u", ktx_data.internal_format);
      assert(0);
      break;
    }
    }

    re_image_init(
        &image->image,
        &(re_image_options_t){
            .width           = ktx_data.pixel_width,
            .height          = ktx_data.pixel_height,
            .layer_count     = ktx_data.face_count,
            .mip_level_count = ktx_data.mipmap_level_count,
            .format          = format,
            .usage = RE_IMAGE_USAGE_SAMPLED | RE_IMAGE_USAGE_TRANSFER_DST,
        });

    upload_ktx(&image->image, g_ctx.transient_command_pool, &ktx_data);

    ktx_data_destroy(&ktx_data);

    return;
  }

  if (strcmp(ext, "png") == 0 || strcmp(ext, "jpeg") == 0 ||
      strcmp(ext, "jpg") == 0) {

    eg_file_t *file = eg_file_open_read(path);
    assert(file);
    size_t raw_data_size = eg_file_size(file);
    uint8_t *raw_data    = calloc(1, raw_data_size);
    eg_file_read_bytes(file, raw_data, raw_data_size);
    eg_file_close(file);

    int width, height, channels;
    uint8_t *data = stbi_load_from_memory(
        raw_data, (int)raw_data_size, &width, &height, &channels, 4);

    free(raw_data);

    re_image_init(
        &image->image,
        &(re_image_options_t){
            .width  = (uint32_t)width,
            .height = (uint32_t)height,
            .usage  = RE_IMAGE_USAGE_SAMPLED | RE_IMAGE_USAGE_TRANSFER_DST,
        });

    re_image_upload(
        &image->image,
        g_ctx.transient_command_pool,
        data,
        (uint32_t)width,
        (uint32_t)height,
        0,
        0);

    free(data);

    return;
  }

  assert(0 && "Unknown image extension");
}
