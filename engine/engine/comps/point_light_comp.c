#include "point_light_comp.h"

void eg_point_light_comp_init(eg_point_light_comp_t *comp, vec4_t color) {
  comp->color = color;
}

void eg_point_light_comp_destroy(eg_point_light_comp_t *comp) {}
