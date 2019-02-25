#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "types.h"
#include <assert.h>
#include <math.h>
#include <stdbool.h>

// @TODO: make compatible with glm version
inline void quat_dot_to(float *res, quat_t left, quat_t right) {
  *res = (left.x * right.x) + (left.y * right.y) + (left.z * right.z) +
         (left.w * right.w);
}

inline float quat_dot(quat_t left, quat_t right) {
  float dot;
  quat_dot_to(&dot, left, right);
  return dot;
}

inline void quat_divs_to(quat_t *res, quat_t left, float right) {
  res->x = left.x / right;
  res->y = left.y / right;
  res->z = left.z / right;
  res->w = left.w / right;
}

inline quat_t quat_divs(quat_t left, float right) {
  quat_t quat;
  quat_divs_to(&quat, left, right);
  return quat;
}

// @TODO: make compatible with glm version
inline void quat_normalize_to(quat_t *res, quat_t left) {
  float length = sqrt(quat_dot(left, left));
  *res = quat_divs(left, length);
}

inline quat_t quat_normalize(quat_t left) {
  quat_t quat;
  quat_normalize_to(&quat, left);
  return quat;
}

// @TODO: make compatible with glm version
inline void quat_from_axis_angle_to(quat_t *res, vec3_t axis, float angle) {
  float s = sin(angle / 2.0f);
  res->x = axis.x * s;
  res->y = axis.y * s;
  res->z = axis.z * s;
  res->w = cos(angle / 2.0f);
}

inline quat_t quat_from_axis_angle(vec3_t axis, float angle) {
  quat_t quat;
  quat_from_axis_angle_to(&quat, axis, angle);
  return quat;
}

inline void quat_to_axis_angle(quat_t quat, vec3_t *axis, float *angle) {
  quat = quat_normalize(quat);
  *angle = 2.0f * acos(quat.w);
  float s = sqrt(1.0f - quat.w * quat.w);
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

// @TODO: make compatible with glm version
inline void quat_conjugate_to(quat_t *res, quat_t left) {
  res->w = left.w;
  res->x = -left.x;
  res->y = -left.y;
  res->z = -left.z;
}

inline quat_t quat_conjugate(quat_t left) {
  quat_t quat;
  quat_conjugate_to(&quat, left);
  return quat;
}

// @TODO: make compatible with glm version
inline void quat_to_mat4_to(mat4_t *res, quat_t left) {
  *res = mat4_identity();

  quat_t normalized_quat = quat_normalize(left);

  float xx, yy, zz, xy, xz, yz, wx, wy, wz;

  xx = normalized_quat.x * normalized_quat.x;
  yy = normalized_quat.y * normalized_quat.y;
  zz = normalized_quat.z * normalized_quat.z;
  xy = normalized_quat.x * normalized_quat.y;
  xz = normalized_quat.x * normalized_quat.z;
  yz = normalized_quat.y * normalized_quat.z;
  wx = normalized_quat.w * normalized_quat.x;
  wy = normalized_quat.w * normalized_quat.y;
  wz = normalized_quat.w * normalized_quat.z;

  res->columns[0][0] = 1.0f - 2.0f * (yy + zz);
  res->columns[0][1] = 2.0f * (xy + wz);
  res->columns[0][2] = 2.0f * (xz - wy);

  res->columns[1][0] = 2.0f * (xy - wz);
  res->columns[1][1] = 1.0f - 2.0f * (xx + zz);
  res->columns[1][2] = 2.0f * (yz + wx);

  res->columns[2][0] = 2.0f * (xz + wy);
  res->columns[2][1] = 2.0f * (yz - wx);
  res->columns[2][2] = 1.0f - 2.0f * (xx + yy);
}

inline mat4_t quat_to_mat4(quat_t left) {
  mat4_t result;
  quat_to_mat4_to(&result, left);
  return result;
}

// @note: compatible with glm::quatLookAt
inline void quat_look_at_to(quat_t *res, vec3_t direction, vec3_t up) {
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

  float biggest_val = sqrt(biggest + 1.0f) * 0.5f;
  float mult = 0.25f / biggest_val;

  switch (biggest_index) {
  case 0:
    *res = quat_t{
        (m[1][2] - m[2][1]) * mult,
        (m[2][0] - m[0][2]) * mult,
        (m[0][1] - m[1][0]) * mult,
        biggest_val,
    };
    return;
  case 1:
    *res = quat_t{
        biggest_val,
        (m[0][1] + m[1][0]) * mult,
        (m[2][0] + m[0][2]) * mult,
        (m[1][2] - m[2][1]) * mult,
    };
    return;
  case 2:
    *res = quat_t{
        (m[0][1] + m[1][0]) * mult,
        biggest_val,
        (m[1][2] + m[2][1]) * mult,
        (m[2][0] - m[0][2]) * mult,
    };
    return;
  case 3:
    *res = quat_t{
        (m[2][0] + m[0][2]) * mult,
        (m[1][2] + m[2][1]) * mult,
        biggest_val,
        (m[0][1] - m[1][0]) * mult,
    };
    return;
  default:
    assert(false);
  }
}

inline quat_t quat_look_at(vec3_t direction, vec3_t up) {
  quat_t res;
  quat_look_at_to(&res, direction, up);
  return res;
}

#ifdef __cplusplus
}
#endif
