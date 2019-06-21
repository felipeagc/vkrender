#include "point_light_comp.h"

#include "../imgui.h"
#include "../serializer.h"

void eg_point_light_comp_default(eg_point_light_comp_t *light) {
  light->color     = (vec4_t){1.0f, 1.0f, 1.0f, 1.0f};
  light->intensity = 1.0f;
}

void eg_point_light_comp_inspect(
    eg_point_light_comp_t *light, eg_inspector_t *inspector) {
  igColorEdit4("Color", &light->color.r, 0);
  igDragFloat("Intensity", &light->intensity, 0.01f, 0.0f, 0.0f, "%.3f", 1.0f);
}

void eg_point_light_comp_destroy(eg_point_light_comp_t *light) {}

void eg_point_light_comp_serialize(
    eg_point_light_comp_t *light, eg_serializer_t *serializer) {
  eg_serializer_append(serializer, &light->color, sizeof(light->color));
  eg_serializer_append(serializer, &light->intensity, sizeof(light->intensity));
}

void eg_point_light_comp_init(
    eg_point_light_comp_t *light, vec4_t color, float intensity) {
  light->color     = color;
  light->intensity = intensity;
}
