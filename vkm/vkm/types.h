#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include <xmmintrin.h>

typedef struct vec3_t {
  float x, y, z;
} vec3_t;

typedef struct VKM_ALIGN(16) vec4_t {
  float x, y, z, w;
} vec4_t;

typedef union VKM_ALIGN(16) mat4_t {
  float columns[4][4];
  float elems[16];
#ifdef VKM_USE_SSE
  __m128 sse_columns[4];
#endif
} mat4_t;

typedef struct quat_t {
  float x, y, z, w;
} quat_t;

#ifdef __cplusplus
}
#endif
