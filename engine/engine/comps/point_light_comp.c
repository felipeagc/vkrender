#include "point_light_comp.h"

#include "../deserializer.h"
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

enum {
  PROP_COLOR,
  PROP_INTENSITY,
  PROP_MAX,
};

void eg_point_light_comp_serialize(
    eg_point_light_comp_t *light, eg_serializer_t *serializer) {
  eg_serializer_append_u32(serializer, PROP_MAX);

  eg_serializer_append_u32(serializer, PROP_COLOR);
  eg_serializer_append(serializer, &light->color, sizeof(light->color));

  eg_serializer_append_u32(serializer, PROP_INTENSITY);
  eg_serializer_append(serializer, &light->intensity, sizeof(light->intensity));
}

void eg_point_light_comp_deserialize(
    eg_point_light_comp_t *light, eg_deserializer_t *deserializer) {
  uint32_t prop_count = eg_deserializer_read_u32(deserializer);

  for (uint32_t i = 0; i < prop_count; i++) {
    uint32_t prop = eg_deserializer_read_u32(deserializer);

    switch (prop) {
    case PROP_COLOR: {
      eg_deserializer_read(deserializer, &light->color, sizeof(light->color));
      break;
    }
    case PROP_INTENSITY: {
      eg_deserializer_read(
          deserializer, &light->intensity, sizeof(light->intensity));
      break;
    }
    default: break;
    }
  }
}

void eg_point_light_comp_init(
    eg_point_light_comp_t *light, vec4_t color, float intensity) {
  light->color     = color;
  light->intensity = intensity;
}
