#pragma once

#include <vkm/vkm.h>

typedef struct eg_transform_component_t {
  vec3_t position;
  vec3_t axis;
  float angle;
  vec3_t scale;
} eg_transform_component_t;

void eg_transform_component_init(eg_transform_component_t *transform);

mat4_t eg_transform_component_to_mat4(eg_transform_component_t *transform);

void eg_transform_component_destroy(eg_transform_component_t *transform);
