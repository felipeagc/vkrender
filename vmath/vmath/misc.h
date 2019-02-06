#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "mat4.h"
#include "vec3.h"
#include "vec4.h"

inline vec3_t vec3_lerp(vec3_t v1, vec3_t v2, float t) {
  return vec3_mul(vec3_adds(v1, t), vec3_sub(v2, v1));
}

inline float to_radians(float degrees) { return degrees * (M_PI / 180.0f); }

inline float to_degrees(float radians) { return radians / (M_PI / 180.0f); }

#ifdef __cplusplus
}
#endif
