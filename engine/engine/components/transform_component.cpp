#include "transform_component.hpp"

void eg_transform_component_init(eg_transform_component_t *transform) {
  transform->position = vec3_zero();
  transform->axis = vec3_t{0.0, 0.0, 1.0};
  transform->angle = 0.0f;
  transform->scale = vec3_one();
}

mat4_t eg_transform_component_to_mat4(eg_transform_component_t *transform) {
  mat4_t mat = mat4_scale(mat4_identity(), transform->scale);
  mat = mat4_rotate(mat, quat_from_axis_angle(transform->axis, transform->angle));
  mat = mat4_translate(mat, transform->position);
  return mat;
}

void eg_transform_component_destroy(eg_transform_component_t *) {}
