#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "mat4.h"
#include "vec3.h"
#include "vec4.h"

inline vec3_t vec3_lerp(vec3_t v1, vec3_t v2, float t) {
  return vec3_add(v1, vec3_muls(vec3_sub(v2, v1), t));
}

inline float lerp(float v1, float v2, float t) { return (1 - t) * v1 + t * v2; }

inline float
remap(float n, float start1, float stop1, float start2, float stop2) {
  return ((n - start1) / (stop1 - start1)) * (stop2 - start2) + start2;
}

inline float clamp(float value, float min_val, float max_val) {
  return fmin(fmax(value, min_val), max_val);
}

inline float to_radians(float degrees) { return degrees * (M_PI / 180.0f); }

inline float to_degrees(float radians) { return radians / (M_PI / 180.0f); }

#ifdef __cplusplus
}
#endif
