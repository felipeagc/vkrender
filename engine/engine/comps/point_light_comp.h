#pragma once

#include <gmath.h>

typedef struct eg_point_light_comp_t {
  vec4_t color;
  float intensity;
} eg_point_light_comp_t;

void eg_point_light_comp_init(
    eg_point_light_comp_t *comp, vec4_t color, float intensity);

void eg_point_light_comp_destroy(eg_point_light_comp_t *comp);

