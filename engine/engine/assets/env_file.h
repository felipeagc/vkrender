#include <assert.h>
#include <stb_image.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ENV_MAX_RADIANCE_MIPMAPS 12

typedef struct env_save_bundle_t {
  unsigned char *data;
  size_t cap;
  size_t size;
} env_save_bundle_t;

typedef struct env_file_header_t {
  uint32_t skybox_layer_sizes[6];
  uint32_t irradiance_layer_sizes[6];
  uint32_t radiance_layer_sizes[ENV_MAX_RADIANCE_MIPMAPS][6];
  uint32_t radiance_mip_count;
} env_file_header_t;

static inline void env_file_read(
    unsigned char *data,
    size_t data_size,
    uint8_t **skybox_data,
    uint32_t *skybox_dim,
    uint8_t **irradiance_data,
    uint32_t *irradiance_dim,
    uint8_t **radiance_data,
    uint32_t *radiance_dim,
    uint32_t *radiance_mip_count) {
  env_file_header_t header;
  memcpy(&header, data, sizeof(header));

  *radiance_mip_count = header.radiance_mip_count;

  size_t current_pos = sizeof(header);

  for (uint32_t layer = 0; layer < 6; layer++) {
    size_t layer_size = header.skybox_layer_sizes[layer];

    int width, height, nr_comps;
    float *layer_data = stbi_loadf_from_memory(
        &data[current_pos], (int)layer_size, &width, &height, &nr_comps, 4);
    *skybox_dim = (uint32_t)width;

    size_t out_layer_size = width * height * 4 * sizeof(float);
    if (layer == 0) {
      *skybox_data = malloc(out_layer_size * 6);
    }
    memcpy(
        &((*skybox_data)[out_layer_size * layer]), layer_data, out_layer_size);

    free(layer_data);

    current_pos += layer_size;
  }

  for (uint32_t layer = 0; layer < 6; layer++) {
    size_t layer_size = header.irradiance_layer_sizes[layer];

    int width, height, nr_comps;
    float *layer_data = stbi_loadf_from_memory(
        &data[current_pos], (int)layer_size, &width, &height, &nr_comps, 4);
    *irradiance_dim = (uint32_t)width;

    size_t out_layer_size = width * height * 4 * sizeof(float);
    if (layer == 0) {
      *irradiance_data = malloc(out_layer_size * 6);
    }
    memcpy(
        &((*irradiance_data)[out_layer_size * layer]),
        layer_data,
        out_layer_size);

    free(layer_data);

    current_pos += layer_size;
  }

  *radiance_data = NULL;
  size_t radiance_size = 0;
  size_t radiance_pos = 0;

  for (uint32_t level = 0; level < header.radiance_mip_count; level++) {
    for (uint32_t layer = 0; layer < 6; layer++) {
      size_t layer_size = header.radiance_layer_sizes[level][layer];

      int width, height, nr_comps;
      float *layer_data = stbi_loadf_from_memory(
          &data[current_pos], (int)layer_size, &width, &height, &nr_comps, 4);

      size_t out_layer_size = width * height * 4 * sizeof(float);
      radiance_size += out_layer_size * 6;
      if (layer == 0) {
        *radiance_data = realloc(*radiance_data, radiance_size);
      }
      memcpy(&((*radiance_data)[radiance_pos]), layer_data, out_layer_size);

      radiance_pos += out_layer_size;

      free(layer_data);

      if (level == 0) {
        *radiance_dim = (uint32_t)width;
      }

      current_pos += layer_size;
    }
  }
}
