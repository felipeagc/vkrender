#ifndef GMATH_H
#define GMATH_H

#ifndef GMATH_DISABLE_SSE
#if defined(__x86_64__) || defined(__i386__)
#define GMATH_USE_SSE
#endif
#endif

#if defined(_MSC_VER)
#define GMATH_ALIGN(x) __declspec(align(x))
#elif defined(__clang__)
#define GMATH_ALIGN(x) __attribute__((aligned(x)))
#elif defined(__GNUC__)
#define GMATH_ALIGN(x) __attribute__((aligned(x)))
#endif

#include <assert.h>
#include <math.h>
#ifdef GMATH_USE_SSE
#include <xmmintrin.h>
#endif

#define GMATH_INLINE static inline

#define GMATH_PI 3.14159265358979323846f

/*
 * gmath types
 */

typedef union vec2_t {
  struct {
    float x, y;
  };
  struct {
    float r, g;
  };
  float v[2];
} vec2_t;

typedef union vec3_t {
  struct {
    float x, y, z;
  };
  struct {
    float r, g, b;
  };
  struct {
    vec2_t xy;
  };
  float v[3];
} vec3_t;

typedef union GMATH_ALIGN(16) vec4_t {
  struct {
    float x, y, z, w;
  };
  struct {
    vec2_t xy;
  };
  struct {
    vec3_t xyz;
  };
  struct {
    float r, g, b, a;
  };
  float v[4];
} vec4_t;

typedef union GMATH_ALIGN(16) mat4_t {
  float columns[4][4];
  float elems[16];
  vec4_t v[4];
#ifdef GMATH_USE_SSE
  __m128 sse_columns[4];
#endif
} mat4_t;

typedef union quat_t {
  struct {
    float x, y, z, w;
  };
  struct {
    vec3_t xyz;
  };
} quat_t;

/*
 * vec2 functions
 */

GMATH_INLINE vec2_t vec2_zero() { return (vec2_t){0.0f, 0.0f}; }

GMATH_INLINE vec2_t vec2_one() { return (vec2_t){1.0f, 1.0f}; }

/*
 * vec3 functions
 */

GMATH_INLINE vec3_t vec3_zero() { return (vec3_t){0.0f, 0.0f, 0.0f}; }

GMATH_INLINE vec3_t vec3_one() { return (vec3_t){1.0f, 1.0f, 1.0f}; }

GMATH_INLINE float vec3_mag(vec3_t vec) {
  return sqrtf((vec.x * vec.x) + (vec.y * vec.y) + (vec.z * vec.z));
}

// @TESTED
GMATH_INLINE vec3_t vec3_add(vec3_t left, vec3_t right) {
  vec3_t result;
  result.x = left.x + right.x;
  result.y = left.y + right.y;
  result.z = left.z + right.z;
  return result;
}

// @TODO: test
GMATH_INLINE vec3_t vec3_adds(vec3_t left, float right) {
  vec3_t result;
  result.x = left.x + right;
  result.y = left.y + right;
  result.z = left.z + right;
  return result;
}

// @TODO: test
GMATH_INLINE vec3_t vec3_sub(vec3_t left, vec3_t right) {
  vec3_t result;
  result.x = left.x - right.x;
  result.y = left.y - right.y;
  result.z = left.z - right.z;
  return result;
}

// @TODO: test
GMATH_INLINE vec3_t vec3_subs(vec3_t left, float right) {
  vec3_t result;
  result.x = left.x - right;
  result.y = left.y - right;
  result.z = left.z - right;
  return result;
}

// @TESTED
GMATH_INLINE vec3_t vec3_mul(vec3_t left, vec3_t right) {
  vec3_t result;
  result.x = left.x * right.x;
  result.y = left.y * right.y;
  result.z = left.z * right.z;
  return result;
}

// @TODO: test
GMATH_INLINE vec3_t vec3_muls(vec3_t left, float right) {
  vec3_t result;
  result.x = left.x * right;
  result.y = left.y * right;
  result.z = left.z * right;
  return result;
}

// @TODO: test
GMATH_INLINE vec3_t vec3_div(vec3_t left, vec3_t right) {
  vec3_t result;
  result.x = left.x / right.x;
  result.y = left.y / right.y;
  result.z = left.z / right.z;
  return result;
}

// @TODO: test
GMATH_INLINE vec3_t vec3_divs(vec3_t left, float right) {
  vec3_t result;
  result.x = left.x / right;
  result.y = left.y / right;
  result.z = left.z / right;
  return result;
}

GMATH_INLINE float vec3_distance(vec3_t left, vec3_t right) {
  return vec3_mag(vec3_sub(left, right));
}

// @TESTED
GMATH_INLINE float vec3_dot(vec3_t left, vec3_t right) {
  return (left.x * right.x) + (left.y * right.y) + (left.z * right.z);
}

// @TODO: test
GMATH_INLINE vec3_t vec3_cross(vec3_t left, vec3_t right) {
  vec3_t result;
  result.x = (left.y * right.z) - (left.z * right.y);
  result.y = (left.z * right.x) - (left.x * right.z);
  result.z = (left.x * right.y) - (left.y * right.x);
  return result;
}

// @TODO: test
// @TODO: make compatible with glm
// @TODO: test with zero norm vector
GMATH_INLINE vec3_t vec3_normalize(vec3_t vec) {
  vec3_t result = vec;
  float norm = sqrtf(vec3_dot(vec, vec));
  if (norm != 0.0f) {
    result = vec3_muls(vec, 1.0f / norm);
  }
  return result;
}

/*
 * vec4 functions
 */

GMATH_INLINE vec4_t vec4_zero() { return (vec4_t){0.0f, 0.0f, 0.0f, 0.0f}; }

GMATH_INLINE vec4_t vec4_one() { return (vec4_t){1.0f, 1.0f, 1.0f, 1.0f}; }

// @TESTED
GMATH_INLINE vec4_t vec4_add(vec4_t left, vec4_t right) {
  vec4_t result;
#ifdef GMATH_USE_SSE
  _mm_store_ps(
      (float *)&result,
      _mm_add_ps(_mm_load_ps((float *)&left), _mm_load_ps((float *)&right)));
#else
  result.x = left.x + right.x;
  result.y = left.y + right.y;
  result.z = left.z + right.z;
  result.w = left.w + right.w;
#endif
  return result;
}

// @TESTED
GMATH_INLINE vec4_t vec4_mul(vec4_t left, vec4_t right) {
  vec4_t result;
#ifdef GMATH_USE_SSE
  _mm_store_ps(
      (float *)&result,
      _mm_mul_ps(_mm_load_ps((float *)&left), _mm_load_ps((float *)&right)));
#else
  result.x = left.x * right.x;
  result.y = left.y * right.y;
  result.z = left.z * right.z;
  result.w = left.w * right.w;
#endif
  return result;
}

GMATH_INLINE vec4_t vec4_muls(vec4_t left, float right) {
  return vec4_mul(left, (vec4_t){right, right, right, right});
}

// @TESTED
GMATH_INLINE float vec4_dot(vec4_t left, vec4_t right) {
  float result;
#ifdef GMATH_USE_SSE
  __m128 rone = _mm_mul_ps(*((__m128 *)&left), *((__m128 *)&right));
  __m128 rtwo = _mm_shuffle_ps(rone, rone, _MM_SHUFFLE(2, 3, 0, 1));
  rone = _mm_add_ps(rone, rtwo);
  rtwo = _mm_shuffle_ps(rone, rone, _MM_SHUFFLE(0, 1, 2, 3));
  rone = _mm_add_ps(rone, rtwo);
  _mm_store_ss(&result, rone);
#else
  result = (left.x * right.x) + (left.y * right.y) + (left.z * right.z) +
           (left.w * right.w);
#endif
  return result;
}

/*
 * mat4 functions
 */

// @TESTED
GMATH_INLINE mat4_t mat4_zero() {
  return (mat4_t){{
      {0, 0, 0, 0},
      {0, 0, 0, 0},
      {0, 0, 0, 0},
      {0, 0, 0, 0},
  }};
}

// @TESTED
GMATH_INLINE mat4_t mat4_diagonal(float f) {
  return (mat4_t){{
      {f, 0, 0, 0},
      {0, f, 0, 0},
      {0, 0, f, 0},
      {0, 0, 0, f},
  }};
}

// @TESTED
GMATH_INLINE mat4_t mat4_identity() { return mat4_diagonal(1.0f); }

// @TESTED
GMATH_INLINE mat4_t mat4_transpose(mat4_t mat) {
  mat4_t result = mat;
#ifdef GMATH_USE_SSE
  _MM_TRANSPOSE4_PS(
      result.sse_columns[0],
      result.sse_columns[1],
      result.sse_columns[2],
      result.sse_columns[3]);
#else
  result.columns[0][1] = mat.columns[1][0];
  result.columns[0][2] = mat.columns[2][0];
  result.columns[0][3] = mat.columns[3][0];

  result.columns[1][0] = mat.columns[0][1];
  result.columns[1][2] = mat.columns[2][1];
  result.columns[1][3] = mat.columns[3][1];

  result.columns[2][0] = mat.columns[0][2];
  result.columns[2][1] = mat.columns[1][2];
  result.columns[2][3] = mat.columns[3][2];

  result.columns[3][0] = mat.columns[0][3];
  result.columns[3][1] = mat.columns[1][3];
  result.columns[3][2] = mat.columns[2][3];
#endif
  return result;
}

// @TESTED
GMATH_INLINE mat4_t mat4_add(mat4_t left, mat4_t right) {
  mat4_t result;
#ifdef GMATH_USE_SSE
  result.sse_columns[0] = _mm_add_ps(left.sse_columns[0], right.sse_columns[0]);
  result.sse_columns[1] = _mm_add_ps(left.sse_columns[1], right.sse_columns[1]);
  result.sse_columns[2] = _mm_add_ps(left.sse_columns[2], right.sse_columns[2]);
  result.sse_columns[3] = _mm_add_ps(left.sse_columns[3], right.sse_columns[3]);
#else
  for (unsigned char i = 0; i < 16; i++) {
    result.elems[i] = left.elems[i] + right.elems[i];
  }
#endif
  return result;
}

// @TESTED
GMATH_INLINE mat4_t mat4_sub(mat4_t left, mat4_t right) {
  mat4_t result;
#ifdef GMATH_USE_SSE
  result.sse_columns[0] = _mm_sub_ps(left.sse_columns[0], right.sse_columns[0]);
  result.sse_columns[1] = _mm_sub_ps(left.sse_columns[1], right.sse_columns[1]);
  result.sse_columns[2] = _mm_sub_ps(left.sse_columns[2], right.sse_columns[2]);
  result.sse_columns[3] = _mm_sub_ps(left.sse_columns[3], right.sse_columns[3]);
#else
  for (unsigned char i = 0; i < 16; i++) {
    result.elems[i] = left.elems[i] - right.elems[i];
  }
#endif
  return result;
}

// @TESTED
GMATH_INLINE mat4_t mat4_muls(mat4_t left, float right) {
  mat4_t result;
#ifdef GMATH_USE_SSE
  __m128 sse_scalar = _mm_load_ps1(&right);
  result.sse_columns[0] = _mm_mul_ps(left.sse_columns[0], sse_scalar);
  result.sse_columns[1] = _mm_mul_ps(left.sse_columns[1], sse_scalar);
  result.sse_columns[2] = _mm_mul_ps(left.sse_columns[2], sse_scalar);
  result.sse_columns[3] = _mm_mul_ps(left.sse_columns[3], sse_scalar);
#else
  for (unsigned char i = 0; i < 16; i++) {
    result.elems[i] = left.elems[i] * right;
  }
#endif
  return result;
}

// @TESTED
GMATH_INLINE mat4_t mat4_divs(mat4_t left, float right) {
  mat4_t result;
#ifdef GMATH_USE_SSE
  __m128 sse_scalar = _mm_load_ps1(&right);
  result.sse_columns[0] = _mm_div_ps(left.sse_columns[0], sse_scalar);
  result.sse_columns[1] = _mm_div_ps(left.sse_columns[1], sse_scalar);
  result.sse_columns[2] = _mm_div_ps(left.sse_columns[2], sse_scalar);
  result.sse_columns[3] = _mm_div_ps(left.sse_columns[3], sse_scalar);
#else
  for (unsigned char i = 0; i < 16; i++) {
    result.elems[i] = left.elems[i] / right;
  }
#endif
  return result;
}

// @TESTED
GMATH_INLINE mat4_t mat4_mul(mat4_t left, mat4_t right) {
  mat4_t result;
#ifdef GMATH_USE_SSE
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
    _mm_store_ps(&result.elems[4 * i], row);
  }
#else
  result = (mat4_t){{{0}}};
  for (unsigned char i = 0; i < 4; i++) {
    for (unsigned char j = 0; j < 4; j++) {
      for (unsigned char p = 0; p < 4; p++) {
        result.columns[i][j] += left.columns[i][p] * right.columns[p][j];
      }
    }
  }
#endif
  return result;
}

GMATH_INLINE vec4_t mat4_mulv(mat4_t left, vec4_t right) {
  vec4_t result;

  // TODO: SIMD version

  result.v[0] =
      left.columns[0][0] * right.v[0] + left.columns[1][0] * right.v[1] +
      left.columns[2][0] * right.v[2] + left.columns[3][0] * right.v[3];
  result.v[1] =
      left.columns[0][1] * right.v[0] + left.columns[1][1] * right.v[1] +
      left.columns[2][1] * right.v[2] + left.columns[3][1] * right.v[3];
  result.v[2] =
      left.columns[0][2] * right.v[0] + left.columns[1][2] * right.v[1] +
      left.columns[2][2] * right.v[2] + left.columns[3][2] * right.v[3];
  result.v[3] =
      left.columns[0][3] * right.v[0] + left.columns[1][3] * right.v[1] +
      left.columns[2][3] * right.v[2] + left.columns[3][3] * right.v[3];

  return result;
}

// @TESTED: compatible with glm
GMATH_INLINE mat4_t
mat4_perspective(float fovy, float aspect_ratio, float znear, float zfar) {
  mat4_t result = mat4_zero();

  float tan_half_fovy = tanf(fovy / 2.0f);

  result.columns[0][0] = 1.0f / (aspect_ratio * tan_half_fovy);
  result.columns[1][1] = 1.0f / tan_half_fovy;
  result.columns[2][2] = -(zfar + znear) / (zfar - znear);
  result.columns[2][3] = -1.0f;
  result.columns[3][2] = -(2.0f * zfar * znear) / (zfar - znear);

  return result;
}

// @TESTED: compatible with glm
GMATH_INLINE mat4_t mat4_look_at(vec3_t eye, vec3_t center, vec3_t up) {
  vec3_t f = vec3_normalize(vec3_sub(center, eye));
  vec3_t s = vec3_normalize(vec3_cross(f, up));
  vec3_t u = vec3_cross(s, f);

  mat4_t result = mat4_identity();

  result.columns[0][0] = s.x;
  result.columns[1][0] = s.y;
  result.columns[2][0] = s.z;

  result.columns[0][1] = u.x;
  result.columns[1][1] = u.y;
  result.columns[2][1] = u.z;

  result.columns[0][2] = -f.x;
  result.columns[1][2] = -f.y;
  result.columns[2][2] = -f.z;

  result.columns[3][0] = -vec3_dot(s, eye);
  result.columns[3][1] = -vec3_dot(u, eye);
  result.columns[3][2] = vec3_dot(f, eye);

  return result;
}

// @TESTED: compatible with glm
GMATH_INLINE quat_t mat4_to_quat(mat4_t mat) {
  quat_t result;
  float trace = mat.columns[0][0] + mat.columns[1][1] + mat.columns[2][2];
  if (trace > 0.0f) {
    float s = sqrtf(1.0f + trace) * 2.0f;
    result.w = 0.25f * s;
    result.x = (mat.columns[1][2] - mat.columns[2][1]) / s;
    result.y = (mat.columns[2][0] - mat.columns[0][2]) / s;
    result.z = (mat.columns[0][1] - mat.columns[1][0]) / s;
  } else if (
      mat.columns[0][0] > mat.columns[1][1] &&
      mat.columns[0][0] > mat.columns[2][2]) {
    float s =
        sqrtf(1.0f + mat.columns[0][0] - mat.columns[1][1] - mat.columns[2][2]) *
        2.0f;
    result.w = (mat.columns[1][2] - mat.columns[2][1]) / s;
    result.x = 0.25f * s;
    result.y = (mat.columns[1][0] + mat.columns[0][1]) / s;
    result.z = (mat.columns[2][0] + mat.columns[0][2]) / s;
  } else if (mat.columns[1][1] > mat.columns[2][2]) {
    float s =
        sqrtf(1.0f + mat.columns[1][1] - mat.columns[0][0] - mat.columns[2][2]) *
        2.0f;
    result.w = (mat.columns[2][0] - mat.columns[0][2]) / s;
    result.x = (mat.columns[1][0] + mat.columns[0][1]) / s;
    result.y = 0.25f * s;
    result.z = (mat.columns[2][1] + mat.columns[1][2]) / s;
  } else {
    float s =
        sqrtf(1.0f + mat.columns[2][2] - mat.columns[0][0] - mat.columns[1][1]) *
        2.0f;
    result.w = (mat.columns[0][1] - mat.columns[1][0]) / s;
    result.x = (mat.columns[2][0] + mat.columns[0][2]) / s;
    result.y = (mat.columns[2][1] + mat.columns[1][2]) / s;
    result.z = 0.25f * s;
  }
  return result;
}

// @TESTED: compatible with glm
GMATH_INLINE mat4_t mat4_translate(mat4_t mat, vec3_t translation) {
  mat4_t result = mat;
  result.columns[3][0] += translation.x;
  result.columns[3][1] += translation.y;
  result.columns[3][2] += translation.z;
  return result;
}

// @TESTED: compatible with glm
GMATH_INLINE mat4_t mat4_scale(mat4_t mat, vec3_t scale) {
  mat4_t result = mat;
  result.columns[0][0] *= scale.x;
  result.columns[1][1] *= scale.y;
  result.columns[2][2] *= scale.z;
  return result;
}

// @TESTED: compatible with glm
GMATH_INLINE mat4_t mat4_rotate(mat4_t mat, float angle, vec3_t axis) {
  float c = cosf(angle);
  float s = sinf(angle);

  axis = vec3_normalize(axis);
  vec3_t temp = vec3_muls(axis, (1.0f - c));

  mat4_t rotate;
  rotate.columns[0][0] = c + temp.x * axis.x;
  rotate.columns[0][1] = temp.x * axis.y + s * axis.z;
  rotate.columns[0][2] = temp.x * axis.z - s * axis.y;

  rotate.columns[1][0] = temp.y * axis.x - s * axis.z;
  rotate.columns[1][1] = c + temp.y * axis.y;
  rotate.columns[1][2] = temp.y * axis.z + s * axis.x;

  rotate.columns[2][0] = temp.z * axis.x + s * axis.y;
  rotate.columns[2][1] = temp.z * axis.y - s * axis.x;
  rotate.columns[2][2] = c + temp.z * axis.z;

  mat4_t result;
  result.v[0] = vec4_add(
      vec4_add(
          vec4_muls(mat.v[0], rotate.columns[0][0]),
          vec4_muls(mat.v[1], rotate.columns[0][1])),
      vec4_muls(mat.v[2], rotate.columns[0][2]));
  result.v[1] = vec4_add(
      vec4_add(
          vec4_muls(mat.v[0], rotate.columns[1][0]),
          vec4_muls(mat.v[1], rotate.columns[1][1])),
      vec4_muls(mat.v[2], rotate.columns[1][2]));
  result.v[2] = vec4_add(
      vec4_add(
          vec4_muls(mat.v[0], rotate.columns[2][0]),
          vec4_muls(mat.v[1], rotate.columns[2][1])),
      vec4_muls(mat.v[2], rotate.columns[2][2]));
  result.v[3] = mat.v[3];
  return result;
}

/*
 * quat functions
 */

// @TESTED: compatible with glm
GMATH_INLINE float quat_dot(quat_t left, quat_t right) {
  return (left.x * right.x) + (left.y * right.y) + (left.z * right.z) +
         (left.w * right.w);
}

// @TESTED: compatible with glm
GMATH_INLINE quat_t quat_normalize(quat_t left) {
  float length = sqrtf(quat_dot(left, left));
  if (length <= 0.0f) {
    return (quat_t){0.0f, 0.0f, 0.0f, 1.0f};
  }
  float one_over_length = 1.0f / length;
  return (quat_t){left.x * one_over_length,
                  left.y * one_over_length,
                  left.z * one_over_length,
                  left.w * one_over_length};
}

// @TESTED: compatible with glm
GMATH_INLINE quat_t quat_from_axis_angle(vec3_t axis, float angle) {
  float s = sinf(angle / 2.0f);
  quat_t result;
  result.x = axis.x * s;
  result.y = axis.y * s;
  result.z = axis.z * s;
  result.w = cosf(angle / 2.0f);
  return result;
}

// @TESTED: compatible with glm
GMATH_INLINE void quat_to_axis_angle(quat_t quat, vec3_t *axis, float *angle) {
  quat = quat_normalize(quat);
  *angle = 2.0f * acosf(quat.w);
  float s = sqrtf(1.0f - quat.w * quat.w);
  if (s < 0.001) {
    axis->x = quat.x;
    axis->y = quat.y;
    axis->z = quat.z;
  } else {
    axis->x = quat.x / s;
    axis->y = quat.y / s;
    axis->z = quat.z / s;
  }
}

// @TESTED: compatible with glm
GMATH_INLINE quat_t quat_conjugate(quat_t quat) {
  quat_t result;
  result.w = quat.w;
  result.x = -quat.x;
  result.y = -quat.y;
  result.z = -quat.z;
  return result;
}

// @TESTED: compatible with glm
GMATH_INLINE mat4_t quat_to_mat4(quat_t quat) {
  mat4_t result = mat4_identity();

  float xx = quat.x * quat.x;
  float yy = quat.y * quat.y;
  float zz = quat.z * quat.z;
  float xy = quat.x * quat.y;
  float xz = quat.x * quat.z;
  float yz = quat.y * quat.z;
  float wx = quat.w * quat.x;
  float wy = quat.w * quat.y;
  float wz = quat.w * quat.z;

  result.columns[0][0] = 1.0f - 2.0f * (yy + zz);
  result.columns[0][1] = 2.0f * (xy + wz);
  result.columns[0][2] = 2.0f * (xz - wy);

  result.columns[1][0] = 2.0f * (xy - wz);
  result.columns[1][1] = 1.0f - 2.0f * (xx + zz);
  result.columns[1][2] = 2.0f * (yz + wx);

  result.columns[2][0] = 2.0f * (xz + wy);
  result.columns[2][1] = 2.0f * (yz - wx);
  result.columns[2][2] = 1.0f - 2.0f * (xx + yy);

  return result;
}

// @TESTED: compatible with glm
GMATH_INLINE quat_t quat_look_at(vec3_t direction, vec3_t up) {
  float m[3][3] = {
      {0, 0, 0},
      {0, 0, 0},
      {0, 0, 0},
  };

  vec3_t col2 = vec3_muls(direction, -1.0f);
  m[2][0] = col2.x;
  m[2][1] = col2.y;
  m[2][2] = col2.z;

  vec3_t col0 = vec3_normalize(vec3_cross(up, col2));
  m[0][0] = col0.x;
  m[0][1] = col0.y;
  m[0][2] = col0.z;

  vec3_t col1 = vec3_cross(col2, col0);
  m[1][0] = col1.x;
  m[1][1] = col1.y;
  m[1][2] = col1.z;

  float x = m[0][0] - m[1][1] - m[2][2];
  float y = m[1][1] - m[0][0] - m[2][2];
  float z = m[2][2] - m[0][0] - m[1][1];
  float w = m[0][0] + m[1][1] + m[2][2];

  int biggest_index = 0;
  float biggest = w;
  if (x > biggest) {
    biggest = x;
    biggest_index = 1;
  }
  if (y > biggest) {
    biggest = y;
    biggest_index = 2;
  }
  if (z > biggest) {
    biggest = z;
    biggest_index = 3;
  }

  float biggest_val = sqrtf(biggest + 1.0f) * 0.5f;
  float mult = 0.25f / biggest_val;

  switch (biggest_index) {
  case 0:
    return (quat_t){
        (m[1][2] - m[2][1]) * mult,
        (m[2][0] - m[0][2]) * mult,
        (m[0][1] - m[1][0]) * mult,
        biggest_val,
    };
  case 1:
    return (quat_t){
        biggest_val,
        (m[0][1] + m[1][0]) * mult,
        (m[2][0] + m[0][2]) * mult,
        (m[1][2] - m[2][1]) * mult,
    };
  case 2:
    return (quat_t){
        (m[0][1] + m[1][0]) * mult,
        biggest_val,
        (m[1][2] + m[2][1]) * mult,
        (m[2][0] - m[0][2]) * mult,
    };
  case 3:
    return (quat_t){
        (m[2][0] + m[0][2]) * mult,
        (m[1][2] + m[2][1]) * mult,
        biggest_val,
        (m[0][1] - m[1][0]) * mult,
    };
  default:
    assert(0 != 0);
  }
}

/*
 * misc functions
 */

GMATH_INLINE vec3_t vec3_lerp(vec3_t v1, vec3_t v2, float t) {
  return vec3_add(v1, vec3_muls(vec3_sub(v2, v1), t));
}

GMATH_INLINE float lerp(float v1, float v2, float t) {
  return (1 - t) * v1 + t * v2;
}

GMATH_INLINE float
remap(float n, float start1, float stop1, float start2, float stop2) {
  return ((n - start1) / (stop1 - start1)) * (stop2 - start2) + start2;
}

GMATH_INLINE float clamp(float value, float min_val, float max_val) {
  return fminf(fmaxf(value, min_val), max_val);
}

GMATH_INLINE float to_radians(float degrees) {
  return degrees * (GMATH_PI / 180.0f);
}

GMATH_INLINE float to_degrees(float radians) {
  return radians / (GMATH_PI / 180.0f);
}

#endif
