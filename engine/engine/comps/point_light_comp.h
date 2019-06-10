#pragma once

#include <gmath.h>

typedef struct eg_inspector_t eg_inspector_t;

typedef struct eg_point_light_comp_t {
  vec4_t color;
  float intensity;
} eg_point_light_comp_t;

/*
 * Required component functions
 */
void eg_point_light_comp_default(eg_point_light_comp_t *comp);

void eg_point_light_comp_inspect(
    eg_point_light_comp_t *comp, eg_inspector_t *inspector);

void eg_point_light_comp_destroy(eg_point_light_comp_t *comp);

/*
 * Specific functions
 */
void eg_point_light_comp_init(
    eg_point_light_comp_t *comp, vec4_t color, float intensity);
