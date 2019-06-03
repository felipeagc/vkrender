#pragma once

#include <gmath.h>

typedef struct eg_inspector_t eg_inspector_t;

typedef struct eg_transform_comp_t {
  vec3_t position;
  vec3_t axis;
  float angle;
  vec3_t scale;
} eg_transform_comp_t;

/*
 * Required component functions
 */
void eg_transform_comp_default(eg_transform_comp_t *transform);

void eg_transform_comp_inspect(
    eg_transform_comp_t *transform, eg_inspector_t *inspector);

void eg_transform_comp_destroy(eg_transform_comp_t *transform);

/*
 * Specific functions
 */
static inline mat4_t
eg_transform_comp_mat4(const eg_transform_comp_t *transform) {
  mat4_t scale = mat4_scale(mat4_identity(), transform->scale);
  mat4_t rotation =
      mat4_rotate(mat4_identity(), transform->angle, transform->axis);
  mat4_t translation = mat4_translate(mat4_identity(), transform->position);

  return mat4_mul(mat4_mul(scale, rotation), translation);
}

