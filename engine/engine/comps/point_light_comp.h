#pragma once

#include <gmath.h>

typedef struct eg_inspector_t eg_inspector_t;
typedef struct eg_serializer_t eg_serializer_t;

typedef struct eg_point_light_comp_t {
  vec4_t color;
  float intensity;
} eg_point_light_comp_t;

/*
 * Required component functions
 */
void eg_point_light_comp_default(eg_point_light_comp_t *light);

void eg_point_light_comp_inspect(
    eg_point_light_comp_t *light, eg_inspector_t *inspector);

void eg_point_light_comp_destroy(eg_point_light_comp_t *light);

void eg_point_light_comp_serialize(
    eg_point_light_comp_t *light, eg_serializer_t *serializer);

/*
 * Specific functions
 */
void eg_point_light_comp_init(
    eg_point_light_comp_t *light, vec4_t color, float intensity);
