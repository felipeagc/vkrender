#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "vec3.h"
#include <math.h>
#include <xmmintrin.h>

typedef union VKM_ALIGN(16) mat4_t {
  float columns[4][4];
  float elems[16];
#ifdef VKM_USE_SSE
  __m128 sse_columns[4];
#endif
} mat4_t;

inline mat4_t mat4_zero() {
  mat4_t mat = {{
      {0, 0, 0, 0},
      {0, 0, 0, 0},
      {0, 0, 0, 0},
      {0, 0, 0, 0},
  }};
  return mat;
}

inline mat4_t mat4_diagonal(float f) {
  mat4_t mat = {{
      {f, 0, 0, 0},
      {0, f, 0, 0},
      {0, 0, f, 0},
      {0, 0, 0, f},
  }};
  return mat;
}

inline mat4_t mat4_identity() { return mat4_diagonal(1.0f); }

inline void mat4_transpose_to(mat4_t *result, mat4_t mat) {
  *result = mat;
#ifdef VKM_USE_SSE
  _MM_TRANSPOSE4_PS(
      result->sse_columns[0],
      result->sse_columns[1],
      result->sse_columns[2],
      result->sse_columns[3]);
#else
  result->columns[0][1] = mat.columns[1][0];
  result->columns[0][2] = mat.columns[2][0];
  result->columns[0][3] = mat.columns[3][0];

  result->columns[1][0] = mat.columns[0][1];
  result->columns[1][2] = mat.columns[2][1];
  result->columns[1][3] = mat.columns[3][1];

  result->columns[2][0] = mat.columns[0][2];
  result->columns[2][1] = mat.columns[1][2];
  result->columns[2][3] = mat.columns[3][2];

  result->columns[3][0] = mat.columns[0][3];
  result->columns[3][1] = mat.columns[1][3];
  result->columns[3][2] = mat.columns[2][3];
#endif
}

inline mat4_t mat4_transpose(mat4_t mat) {
  mat4_t result;
  mat4_transpose_to(&result, mat);
  return result;
}

inline void mat4_add_to(mat4_t *res, mat4_t left, mat4_t right) {
#ifdef VKM_USE_SSE
  res->sse_columns[0] = _mm_add_ps(left.sse_columns[0], right.sse_columns[0]);
  res->sse_columns[1] = _mm_add_ps(left.sse_columns[1], right.sse_columns[1]);
  res->sse_columns[2] = _mm_add_ps(left.sse_columns[2], right.sse_columns[2]);
  res->sse_columns[3] = _mm_add_ps(left.sse_columns[3], right.sse_columns[3]);
#else
  for (unsigned char i = 0; i < 16; i++) {
    res->elems[i] = left.elems[i] + right.elems[i];
  }
#endif
}

inline mat4_t mat4_add(mat4_t left, mat4_t right) {
  mat4_t result;
  mat4_add_to(&result, left, right);
  return result;
}

inline void mat4_sub_to(mat4_t *res, mat4_t left, mat4_t right) {
#ifdef VKM_USE_SSE
  res->sse_columns[0] = _mm_sub_ps(left.sse_columns[0], right.sse_columns[0]);
  res->sse_columns[1] = _mm_sub_ps(left.sse_columns[1], right.sse_columns[1]);
  res->sse_columns[2] = _mm_sub_ps(left.sse_columns[2], right.sse_columns[2]);
  res->sse_columns[3] = _mm_sub_ps(left.sse_columns[3], right.sse_columns[3]);
#else
  for (unsigned char i = 0; i < 16; i++) {
    res->elems[i] = left.elems[i] - right.elems[i];
  }
#endif
}

inline mat4_t mat4_sub(mat4_t left, mat4_t right) {
  mat4_t result;
  mat4_sub_to(&result, left, right);
  return result;
}

inline void mat4_muls_to(mat4_t *res, mat4_t left, float right) {
#ifdef VKM_USE_SSE
  __m128 sse_scalar = _mm_load_ps1(&right);
  res->sse_columns[0] = _mm_mul_ps(left.sse_columns[0], sse_scalar);
  res->sse_columns[1] = _mm_mul_ps(left.sse_columns[1], sse_scalar);
  res->sse_columns[2] = _mm_mul_ps(left.sse_columns[2], sse_scalar);
  res->sse_columns[3] = _mm_mul_ps(left.sse_columns[3], sse_scalar);
#else
  for (unsigned char i = 0; i < 16; i++) {
    res->elems[i] = left.elems[i] * right;
  }
#endif
}

inline mat4_t mat4_muls(mat4_t left, float right) {
  mat4_t result;
  mat4_muls_to(&result, left, right);
  return result;
}

inline void mat4_divs_to(mat4_t *res, mat4_t left, float right) {
#ifdef VKM_USE_SSE
  __m128 sse_scalar = _mm_load_ps1(&right);
  res->sse_columns[0] = _mm_div_ps(left.sse_columns[0], sse_scalar);
  res->sse_columns[1] = _mm_div_ps(left.sse_columns[1], sse_scalar);
  res->sse_columns[2] = _mm_div_ps(left.sse_columns[2], sse_scalar);
  res->sse_columns[3] = _mm_div_ps(left.sse_columns[3], sse_scalar);
#else
  for (unsigned char i = 0; i < 16; i++) {
    res->elems[i] = left.elems[i] / right;
  }
#endif
}

inline mat4_t mat4_divs(mat4_t left, float right) {
  mat4_t result;
  mat4_divs_to(&result, left, right);
  return result;
}

inline void mat4_mul_to(mat4_t *res, mat4_t left, mat4_t right) {
#ifdef VKM_USE_SSE
  for (int i = 0; i < 4; i++) {
    __m128 brod1 = _mm_set1_ps(left.elems[4 * i + 0]);
    __m128 brod2 = _mm_set1_ps(left.elems[4 * i + 1]);
    __m128 brod3 = _mm_set1_ps(left.elems[4 * i + 2]);
    __m128 brod4 = _mm_set1_ps(left.elems[4 * i + 3]);
    __m128 row = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(brod1, right.sse_columns[0]),
            _mm_mul_ps(brod2, right.sse_columns[1])),
        _mm_add_ps(
            _mm_mul_ps(brod3, right.sse_columns[2]),
            _mm_mul_ps(brod4, right.sse_columns[3])));
    _mm_store_ps(&res->elems[4 * i], row);
  }
#else
  *res = {{{0}}};
  for (unsigned char i = 0; i < 4; i++) {
    for (unsigned char j = 0; j < 4; j++) {
      for (unsigned char p = 0; p < 4; p++) {
        res->columns[i][j] += left.columns[i][p] * right.columns[p][j];
      }
    }
  }
#endif
}

inline mat4_t mat4_mul(mat4_t left, mat4_t right) {
  mat4_t result;
  mat4_mul_to(&result, left, right);
  return result;
}

// @note: compatible with glm::perspective
inline void mat4_perspective_to(
    mat4_t *res, float fovy, float aspect_ratio, float znear, float zfar) {
  *res = mat4_diagonal(0.0f);
  float tan_half_fovy = tan(fovy / 2.0f);

  res->columns[0][0] = 1.0f / (aspect_ratio * tan_half_fovy);
  res->columns[1][1] = 1.0f / tan_half_fovy;
  res->columns[2][2] = zfar / (znear - zfar);
  res->columns[2][3] = -1.0f;
  res->columns[3][2] = -(zfar * znear) / (zfar - znear);
}

inline mat4_t
mat4_perspective(float fovy, float aspect_ratio, float znear, float zfar) {
  mat4_t mat;
  mat4_perspective_to(&mat, fovy, aspect_ratio, znear, zfar);
  return mat;
}

// @note: compatible with glm::lookAt
inline void mat4_look_at_to(mat4_t *res, vec3_t eye, vec3_t center, vec3_t up) {
  vec3_t f = vec3_normalize(vec3_sub(center, eye));
  vec3_t s = vec3_normalize(vec3_cross(f, up));
  vec3_t u = vec3_cross(s, f);

  *res = mat4_identity();

  res->columns[0][0] = s.x;
  res->columns[1][0] = s.y;
  res->columns[2][0] = s.z;

  res->columns[0][1] = u.x;
  res->columns[1][1] = u.y;
  res->columns[2][1] = u.z;

  res->columns[0][2] = -f.x;
  res->columns[1][2] = -f.y;
  res->columns[2][2] = -f.z;

  res->columns[3][0] = -vec3_dot(s, eye);
  res->columns[3][1] = -vec3_dot(u, eye);
  res->columns[3][2] = vec3_dot(f, eye);
}

inline mat4_t mat4_look_at(vec3_t eye, vec3_t center, vec3_t up) {
  mat4_t result;
  mat4_look_at_to(&result, eye, center, up);
  return result;
}

// @todo: make compatible with glm version
inline mat4_t mat4_translation(vec3_t translation) {
  mat4_t result = mat4_identity();

  result.columns[3][0] = translation.x;
  result.columns[3][1] = translation.y;
  result.columns[3][2] = translation.z;

  return result;
}

#ifdef __cplusplus
}
#endif
