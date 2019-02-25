#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "types.h"

inline vec4_t vec4_zero() { return {0.0f, 0.0f, 0.0f, 0.0f}; }

inline vec4_t vec4_one() { return {1.0f, 1.0f, 1.0f, 1.0f}; }

inline void vec4_add_to(vec4_t *dest, vec4_t left, vec4_t right) {
#ifdef VKM_USE_SSE
  _mm_store_ps(
      (float *)dest,
      _mm_add_ps(_mm_load_ps((float *)&left), _mm_load_ps((float *)&right)));
#else
  dest->x = left.x + right.x;
  dest->y = left.y + right.y;
  dest->z = left.z + right.z;
  dest->w = left.w + right.w;
#endif
}

inline vec4_t vec4_add(vec4_t left, vec4_t right) {
  vec4_t result;
  vec4_add_to(&result, left, right);
  return result;
}

inline void vec4_mul_to(vec4_t *dest, vec4_t left, vec4_t right) {
#ifdef VKM_USE_SSE
  _mm_store_ps(
      (float *)dest,
      _mm_mul_ps(_mm_load_ps((float *)&left), _mm_load_ps((float *)&right)));
#else
  dest->x = left.x * right.x;
  dest->y = left.y * right.y;
  dest->z = left.z * right.z;
  dest->w = left.w * right.w;
#endif
}

inline vec4_t vec4_mul(vec4_t left, vec4_t right) {
  vec4_t result;
  vec4_mul_to(&result, left, right);
  return result;
}

inline void vec4_dot_to(float *dest, vec4_t left, vec4_t right) {
#ifdef VKM_USE_SSE
  __m128 rone = _mm_mul_ps(*((__m128 *)&left), *((__m128 *)&right));
  __m128 rtwo = _mm_shuffle_ps(rone, rone, _MM_SHUFFLE(2, 3, 0, 1));
  rone = _mm_add_ps(rone, rtwo);
  rtwo = _mm_shuffle_ps(rone, rone, _MM_SHUFFLE(0, 1, 2, 3));
  rone = _mm_add_ps(rone, rtwo);
  _mm_store_ss(dest, rone);
#else
  *dest = (left.x * right.x) + (left.y * right.y) + (left.z * right.z) +
          (left.w * right.w);
#endif
}

inline float vec4_dot(vec4_t left, vec4_t right) {
  float result;
  vec4_dot_to(&result, left, right);
  return result;
}

#ifdef __cplusplus
}
#endif
