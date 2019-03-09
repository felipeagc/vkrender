#include "transform_component.h"

void eg_transform_component_init(eg_transform_component_t *transform) {
  transform->position = vec3_zero();
  transform->axis = (vec3_t){0.0, 0.0, 1.0};
  transform->angle = 0.0f;
  transform->scale = vec3_one();
}

mat4_t eg_transform_component_to_mat4(eg_transform_component_t *transform) {
  mat4_t scale = mat4_scale(mat4_identity(), transform->scale);
  mat4_t rotation = mat4_rotate(mat4_identity(), transform->angle, transform->axis);
  mat4_t translation = mat4_translate(mat4_identity(), transform->position);

  return mat4_mul(mat4_mul(scale, rotation), translation);
}

void eg_transform_component_destroy(eg_transform_component_t *transform) {}
