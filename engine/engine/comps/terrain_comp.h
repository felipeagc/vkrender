#pragma once

#include <renderer/image.h>

typedef struct eg_terrain_comp_t {
  re_image_t heightmap;
} eg_terrain_comp_t;

void eg_terrain_comp_init(
    eg_terrain_comp_t *terrain, uint32_t width, uint32_t height);

void eg_terrain_comp_destroy(eg_terrain_comp_t *terrain);
