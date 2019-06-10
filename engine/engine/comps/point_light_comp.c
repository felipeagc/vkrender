#include "point_light_comp.h"

#include "../imgui.h"

void eg_point_light_comp_default(eg_point_light_comp_t *comp) {
  comp->color     = (vec4_t){1.0f, 1.0f, 1.0f, 1.0f};
  comp->intensity = 1.0f;
}

void eg_point_light_comp_inspect(
    eg_point_light_comp_t *comp, eg_inspector_t *inspector) {
  igColorEdit4("Color", &comp->color.r, 0);
  igDragFloat("Intensity", &comp->intensity, 0.01f, 0.0f, 0.0f, "%.3f", 1.0f);
}

void eg_point_light_comp_destroy(eg_point_light_comp_t *comp) {}

void eg_point_light_comp_init(
    eg_point_light_comp_t *comp, vec4_t color, float intensity) {
  comp->color     = color;
  comp->intensity = intensity;
}
