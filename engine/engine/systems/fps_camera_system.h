#pragma once

#include <gmath.h>
#include <stdbool.h>

typedef struct re_window_t re_window_t;
typedef union SDL_Event SDL_Event;
typedef struct eg_camera_t eg_camera_t;

typedef struct eg_fps_camera_system_t {
  vec3_t cam_target;

  vec3_t cam_up;
  vec3_t cam_front;
  vec3_t cam_right;
  float cam_yaw;
  float cam_pitch;

  int prev_mouse_x;
  int prev_mouse_y;

  float sensitivity;

  bool first_frame;
  bool transitioning_fov;

  float time;
} eg_fps_camera_system_t;

void eg_fps_camera_system_init(eg_fps_camera_system_t *system);

void eg_fps_camera_system_process_event(
    eg_fps_camera_system_t *system,
    re_window_t *window,
    SDL_Event *event);

void eg_fps_camera_system_update(
    eg_fps_camera_system_t *system,
    re_window_t *window,
    eg_camera_t *camera);
