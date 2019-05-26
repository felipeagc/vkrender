#include "terrain_comp.h"

#include <renderer/context.h>
#include <stdlib.h>

void eg_terrain_comp_init(
    eg_terrain_comp_t *terrain, uint32_t width, uint32_t height) {
  re_image_init(
      &terrain->heightmap,
      &(re_image_options_t){
          .width = width,
          .height = height,
          .format = VK_FORMAT_R32_SFLOAT,
      });

  float *data = calloc(width * height, sizeof(float));

  re_image_upload(
      &terrain->heightmap,
      g_ctx.transient_command_pool,
      (uint8_t *)data,
      width,
      height,
      0,
      0);

  free(data);
}

void eg_terrain_comp_destroy(eg_terrain_comp_t *terrain) {
  re_image_destroy(&terrain->heightmap);
}
