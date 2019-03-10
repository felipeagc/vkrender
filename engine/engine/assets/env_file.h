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

typedef struct env_file_read_options_t {
  uint32_t skybox_dim;
  float *skybox_layers[6];
  uint32_t irradiance_dim;
  float *irradiance_layers[6];
  uint32_t base_radiance_dim;
  float *radiance_layers[ENV_MAX_RADIANCE_MIPMAPS][6];
  uint32_t radiance_mip_count;
  const char *path;
} env_file_read_options_t;

static inline void env_file_read(env_file_read_options_t *options) {
  FILE *file = fopen(options->path, "r");
  fseek(file, 0, SEEK_END);
  size_t data_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  unsigned char *data = calloc(1, data_size);

  fread(data, data_size, 1, file);

  fclose(file);

  env_file_header_t header;
  memcpy(&header, data, sizeof(header));

  options->radiance_mip_count = header.radiance_mip_count;

  size_t current_pos = sizeof(header);

  for (uint32_t layer = 0; layer < 6; layer++) {
    size_t layer_size = header.skybox_layer_sizes[layer];

    int width, height, nr_comps;
    options->skybox_layers[layer] = stbi_loadf_from_memory(
        &data[current_pos], layer_size, &width, &height, &nr_comps, 4);
    options->skybox_dim = (uint32_t)width;

    current_pos += layer_size;
  }

  for (uint32_t layer = 0; layer < 6; layer++) {
    size_t layer_size = header.irradiance_layer_sizes[layer];

    int width, height, nr_comps;
    options->irradiance_layers[layer] = stbi_loadf_from_memory(
        &data[current_pos], layer_size, &width, &height, &nr_comps, 4);
    options->irradiance_dim = (uint32_t)width;

    current_pos += layer_size;
  }

  for (uint32_t level = 0; level < header.radiance_mip_count; level++) {
    for (uint32_t layer = 0; layer < 6; layer++) {
      size_t layer_size = header.radiance_layer_sizes[level][layer];

      int width, height, nr_comps;
      options->radiance_layers[level][layer] = stbi_loadf_from_memory(
          &data[current_pos], layer_size, &width, &height, &nr_comps, 4);

      if (level == 0) {
        options->base_radiance_dim = (uint32_t)width;
      }

      current_pos += layer_size;
    }
  }

  free(data);
}
