#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include <xmmintrin.h>

typedef struct VMATH_ALIGN(16) {
  float x, y, z, w;
} vec4_t;

inline vec4_t vec4_zero() { return {0.0f, 0.0f, 0.0f, 0.0f}; }

inline vec4_t vec4_one() { return {1.0f, 1.0f, 1.0f, 1.0f}; }

inline void vec4_add_to(vec4_t *dest, vec4_t *v1, vec4_t *v2) {
#ifdef VMATH_USE_SSE
  _mm_store_ps(
      (float *)dest,
      _mm_add_ps(_mm_load_ps((float *)v1), _mm_load_ps((float *)v2)));
#else
  dest->x = v1->x + v2->x;
  dest->y = v1->y + v2->y;
  dest->z = v1->z + v2->z;
  dest->w = v1->w + v2->w;
#endif
}

inline vec4_t vec4_add(vec4_t *v1, vec4_t *v2) {
  vec4_t result;
  vec4_add_to(&result, v1, v2);
  return result;
}

inline void vec4_mul_to(vec4_t *dest, vec4_t *v1, vec4_t *v2) {
#ifdef VMATH_USE_SSE
  _mm_store_ps(
      (float *)dest,
      _mm_mul_ps(_mm_load_ps((float *)v1), _mm_load_ps((float *)v2)));
#else
  dest->x = v1->x * v2->x;
  dest->y = v1->y * v2->y;
  dest->z = v1->z * v2->z;
  dest->w = v1->w * v2->w;
#endif
}

inline vec4_t vec4_mul(vec4_t *v1, vec4_t *v2) {
  vec4_t result;
  vec4_mul_to(&result, v1, v2);
  return result;
}

inline void vec4_dot_to(float *dest, vec4_t *v1, vec4_t *v2) {
#ifdef VMATH_USE_SSE
  __m128 rone = _mm_mul_ps(*((__m128 *)v1), *((__m128 *)v2));
  __m128 rtwo = _mm_shuffle_ps(rone, rone, _MM_SHUFFLE(2, 3, 0, 1));
  rone = _mm_add_ps(rone, rtwo);
  rtwo = _mm_shuffle_ps(rone, rone, _MM_SHUFFLE(0, 1, 2, 3));
  rone = _mm_add_ps(rone, rtwo);
  _mm_store_ss(dest, rone);
#else
  *dest = (v1->x * v2->x) + (v1->y * v2->y) + (v1->z * v2->z) + (v1->w * v2->w);
#endif
}

inline float vec4_dot(vec4_t *v1, vec4_t *v2) {
  float result;
  vec4_dot_to(&result, v1, v2);
  return result;
}

#ifdef __cplusplus
}
#endif
