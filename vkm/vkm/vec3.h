#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include <math.h>

inline vec3_t vec3_zero() { return {0.0f, 0.0f, 0.0f}; }

inline vec3_t vec3_one() { return {1.0f, 1.0f, 1.0f}; }

inline void vec3_add_to(vec3_t *dest, vec3_t left, vec3_t right) {
  dest->x = left.x + right.x;
  dest->y = left.y + right.y;
  dest->z = left.z + right.z;
}

inline vec3_t vec3_add(vec3_t left, vec3_t right) {
  vec3_t result;
  vec3_add_to(&result, left, right);
  return result;
}

inline void vec3_adds_to(vec3_t *dest, vec3_t left, float right) {
  dest->x = left.x + right;
  dest->y = left.y + right;
  dest->z = left.z + right;
}

inline vec3_t vec3_adds(vec3_t left, float right) {
  vec3_t result;
  vec3_adds_to(&result, left, right);
  return result;
}

inline void vec3_sub_to(vec3_t *dest, vec3_t left, vec3_t right) {
  dest->x = left.x - right.x;
  dest->y = left.y - right.y;
  dest->z = left.z - right.z;
}

inline vec3_t vec3_sub(vec3_t left, vec3_t right) {
  vec3_t result;
  vec3_sub_to(&result, left, right);
  return result;
}

inline void vec3_subs_to(vec3_t *dest, vec3_t left, float right) {
  dest->x = left.x - right;
  dest->y = left.y - right;
  dest->z = left.z - right;
}

inline vec3_t vec3_subs(vec3_t left, float right) {
  vec3_t result;
  vec3_subs_to(&result, left, right);
  return result;
}

inline void vec3_mul_to(vec3_t *dest, vec3_t left, vec3_t right) {
  dest->x = left.x * right.x;
  dest->y = left.y * right.y;
  dest->z = left.z * right.z;
}

inline vec3_t vec3_mul(vec3_t left, vec3_t right) {
  vec3_t result;
  vec3_mul_to(&result, left, right);
  return result;
}

inline void vec3_muls_to(vec3_t *dest, vec3_t left, float right) {
  dest->x = left.x * right;
  dest->y = left.y * right;
  dest->z = left.z * right;
}

inline vec3_t vec3_muls(vec3_t left, float right) {
  vec3_t result;
  vec3_muls_to(&result, left, right);
  return result;
}
inline void vec3_div_to(vec3_t *dest, vec3_t left, vec3_t right) {
  dest->x = left.x / right.x;
  dest->y = left.y / right.y;
  dest->z = left.z / right.z;
}

inline vec3_t vec3_div(vec3_t left, vec3_t right) {
  vec3_t result;
  vec3_div_to(&result, left, right);
  return result;
}

inline void vec3_divs_to(vec3_t *dest, vec3_t left, float right) {
  dest->x = left.x / right;
  dest->y = left.y / right;
  dest->z = left.z / right;
}

inline vec3_t vec3_divs(vec3_t left, float right) {
  vec3_t result;
  vec3_divs_to(&result, left, right);
  return result;
}

inline void vec3_dot_to(float *res, vec3_t left, vec3_t right) {
  *res = (left.x * right.x) + (left.y * right.y) + (left.z * right.z);
}

inline float vec3_dot(vec3_t left, vec3_t right) {
  float result;
  vec3_dot_to(&result, left, right);
  return result;
}

inline void vec3_cross_to(vec3_t *res, vec3_t left, vec3_t right) {
  res->x = (left.y * right.z) - (left.z * right.y);
  res->y = (left.z * right.x) - (left.x * right.z);
  res->z = (left.x * right.y) - (left.y * right.x);
}

inline vec3_t vec3_cross(vec3_t left, vec3_t right) {
  vec3_t result;
  vec3_cross_to(&result, left, right);
  return result;
}

inline void vec3_normalize_to(vec3_t *res, vec3_t left) {
  float norm = sqrt(vec3_dot(left, left));
  if (norm != 0.0f) {
    *res = vec3_muls(left, 1.0f / norm);
  }
}

inline vec3_t vec3_normalize(vec3_t left) {
  vec3_t result;
  vec3_normalize_to(&result, left);
  return result;
}

#ifdef __cplusplus
}
#endif
