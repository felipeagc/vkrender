#pragma once

#include <gmath.h>

typedef struct eg_transform_comp_t {
  vec3_t position;
  vec3_t axis;
  float angle;
  vec3_t scale;
} eg_transform_comp_t;

void eg_transform_comp_init(eg_transform_comp_t *transform);

mat4_t eg_transform_comp_mat4(eg_transform_comp_t *transform);

void eg_transform_comp_destroy(eg_transform_comp_t *transform);
