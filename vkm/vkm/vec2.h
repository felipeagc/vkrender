#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vec2_t {
  float x, y;
} vec2_t;

inline vec2_t vec2_zero() { return vec2_t{0.0f, 0.0f}; }

inline vec2_t vec2_one() { return vec2_t{1.0f, 1.0f}; }

#ifdef __cplusplus
}
#endif
