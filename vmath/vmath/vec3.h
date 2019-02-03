#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  float x, y, z;
} vec3_t;

inline vec3_t vec3_zero() { return {0.0f, 0.0f, 0.0f}; }

inline vec3_t vec3_one() { return {1.0f, 1.0f, 1.0f}; }

inline void vec3_add_to(vec3_t *dest, vec3_t *v1, vec3_t *v2) {
  dest->x = v1->x + v2->x;
  dest->y = v1->y + v2->y;
  dest->z = v1->z + v2->z;
}

inline vec3_t vec3_add(vec3_t *v1, vec3_t *v2) {
  vec3_t result;
  vec3_add_to(&result, v1, v2);
  return result;
}

inline void vec3_mul_to(vec3_t *dest, vec3_t *v1, vec3_t *v2) {
  dest->x = v1->x * v2->x;
  dest->y = v1->y * v2->y;
  dest->z = v1->z * v2->z;
}

inline vec3_t vec3_mul(vec3_t *v1, vec3_t *v2) {
  vec3_t result;
  vec3_mul_to(&result, v1, v2);
  return result;
}

inline void vec3_dot_to(float *dest, vec3_t *v1, vec3_t *v2) {
  *dest = (v1->x * v2->x) + (v1->y * v2->y) + (v1->z * v2->z);
}

inline float vec3_dot(vec3_t *v1, vec3_t *v2) {
  float result;
  vec3_dot_to(&result, v1, v2);
  return result;
}

#ifdef __cplusplus
}
#endif
