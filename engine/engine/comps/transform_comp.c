#include "transform_comp.h"

void eg_transform_comp_init(eg_transform_comp_t *transform) {
  transform->position = vec3_zero();
  transform->axis = (vec3_t){0.0, 0.0, 1.0};
  transform->angle = 0.0f;
  transform->scale = vec3_one();
}

void eg_transform_comp_destroy(eg_transform_comp_t *transform) {}
