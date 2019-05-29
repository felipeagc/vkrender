#pragma once

#include <gmath.h>
#include <renderer/cmd_buffer.h>
#include <stdbool.h>

typedef struct re_window_t re_window_t;
typedef struct eg_camera_t eg_camera_t;

typedef struct eg_fps_camera_system_t {
  eg_camera_t *camera;

  vec3_t cam_target;

  vec3_t cam_up;
  vec3_t cam_front;
  vec3_t cam_right;
  float cam_yaw;
  float cam_pitch;

  double prev_normal_cursor_x;
  double prev_normal_cursor_y;

  double prev_disabled_cursor_x;
  double prev_disabled_cursor_y;

  bool prev_right_pressed;

  float sensitivity;

  float time;
} eg_fps_camera_system_t;

void eg_fps_camera_system_init(
    eg_fps_camera_system_t *system, eg_camera_t *camera);

void eg_fps_camera_system_update(
    eg_fps_camera_system_t *system,
    re_window_t *window,
    re_cmd_buffer_t *cmd_buffer,
    float width,
    float height);
