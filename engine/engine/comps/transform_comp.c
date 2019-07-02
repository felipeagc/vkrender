#include "transform_comp.h"

#include "../deserializer.h"
#include "../imgui.h"
#include "../serializer.h"

void eg_transform_comp_default(eg_transform_comp_t *transform) {
  transform->position = vec3_zero();
  transform->axis     = (vec3_t){0.0, 0.0, 1.0};
  transform->angle    = 0.0f;
  transform->scale    = vec3_one();
}

void eg_transform_comp_inspect(
    eg_transform_comp_t *transform, eg_inspector_t *inspector) {
  igDragFloat3(
      "Position", &transform->position.x, 0.1f, 0.0f, 0.0f, "%.3f", 1.0f);
  igDragFloat3("Scale", &transform->scale.x, 0.1f, 0.0f, 0.0f, "%.3f", 1.0f);
  igDragFloat3("Axis", &transform->axis.x, 0.01f, 0.0f, 1.0f, "%.3f", 1.0f);
  igDragFloat("Angle", &transform->angle, 0.01f, 0.0f, 0.0f, "%.3f rad", 1.0f);
}

void eg_transform_comp_destroy(eg_transform_comp_t *transform) {}

enum {
  PROP_POS,
  PROP_AXIS,
  PROP_ANGLE,
  PROP_SCALE,
  PROP_MAX,
};

void eg_transform_comp_serialize(
    eg_transform_comp_t *transform, eg_serializer_t *serializer) {
  eg_serializer_append_u32(serializer, PROP_MAX);

  eg_serializer_append_u32(serializer, PROP_POS);
  eg_serializer_append(
      serializer, &transform->position, sizeof(transform->position));

  eg_serializer_append_u32(serializer, PROP_AXIS);
  eg_serializer_append(serializer, &transform->axis, sizeof(transform->axis));

  eg_serializer_append_u32(serializer, PROP_ANGLE);
  eg_serializer_append(serializer, &transform->angle, sizeof(transform->angle));

  eg_serializer_append_u32(serializer, PROP_SCALE);
  eg_serializer_append(serializer, &transform->scale, sizeof(transform->scale));
}

void eg_transform_comp_deserialize(
    eg_transform_comp_t *transform, eg_deserializer_t *deserializer) {
  uint32_t prop_count = eg_deserializer_read_u32(deserializer);

  for (uint32_t i = 0; i < prop_count; i++) {
    uint32_t prop = eg_deserializer_read_u32(deserializer);

    switch (prop) {
    case PROP_POS: {
      eg_deserializer_read(
          deserializer, &transform->position, sizeof(transform->position));
      break;
    }
    case PROP_AXIS: {
      eg_deserializer_read(
          deserializer, &transform->axis, sizeof(transform->axis));
      break;
    }
    case PROP_ANGLE: {
      eg_deserializer_read(
          deserializer, &transform->angle, sizeof(transform->angle));
      break;
    }
    case PROP_SCALE: {
      eg_deserializer_read(
          deserializer, &transform->scale, sizeof(transform->scale));
      break;
    }
    default: break;
    }
  }
}
