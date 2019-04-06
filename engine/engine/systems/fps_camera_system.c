#include "fps_camera_system.h"
#include "../camera.h"
#include <SDL.h>
//#include <renderer/imgui.h>
#include <renderer/window.h>
#include <util/log.h>

#define INITIAL_FOV to_radians(160)
#define GOAL_FOV to_radians(70)
#define TRANSITION_TIME 1.0f

float out_expo(float t, float b, float c, float d) {
  if (t == d)
    return b + c;
  return c * 1.001f * (-powf(2, -10 * t / d) + 1) + b;
}

void eg_fps_camera_system_init(eg_fps_camera_system_t *system) {
  system->cam_target = (vec3_t){0};

  system->cam_up = (vec3_t){0};
  system->cam_front = (vec3_t){0.0, 0.0, 1.0};
  system->cam_right = (vec3_t){0};

  system->cam_yaw = to_radians(90.0f);
  system->cam_pitch = 0.0f;

  system->prev_mouse_x = 0;
  system->prev_mouse_y = 0;

  system->first_frame = true;

  system->sensitivity = 0.07f;

  system->time = 0.0f;
  system->transitioning_fov = true;
}

void eg_fps_camera_system_process_event(
    eg_fps_camera_system_t *system,
    re_window_t *window,
    SDL_Event *event) {
  switch (event->type) {
  case SDL_MOUSEBUTTONDOWN:
    if (event->button.button == SDL_BUTTON_RIGHT) {
      re_window_get_mouse_state(
          window, &system->prev_mouse_x, &system->prev_mouse_y);
      re_window_set_relative_mouse(window, true);
    }
    break;
  case SDL_MOUSEBUTTONUP:
    if (event->button.button == SDL_BUTTON_RIGHT) {
      re_window_set_relative_mouse(window, false);
      re_window_warp_mouse(window, system->prev_mouse_x, system->prev_mouse_y);
    }
    break;
  case SDL_MOUSEMOTION:
    if (event->motion.state & SDL_BUTTON_RMASK) {
      int dx = event->motion.xrel;
      int dy = event->motion.yrel;

      if (re_window_get_relative_mouse(window)) {
        system->cam_yaw += to_radians((float)dx) * system->sensitivity;
        system->cam_pitch -= to_radians((float)dy) * system->sensitivity;
        system->cam_pitch =
            clamp(system->cam_pitch, to_radians(-89.0f), to_radians(89.0f));
      }
    }
    break;
  }
}

void eg_fps_camera_system_update(
    eg_fps_camera_system_t *system,
    re_window_t *window,
    eg_camera_t *camera) {
  system->time += (float) window->delta_time;

  if (system->first_frame) {
    system->first_frame = false;
    system->cam_target = camera->position;
    camera->fov = INITIAL_FOV;
  }

  system->cam_front.x = cosf(system->cam_yaw) * cosf(system->cam_pitch);
  system->cam_front.y = sinf(system->cam_pitch);
  system->cam_front.z = sinf(system->cam_yaw) * cosf(system->cam_pitch);
  system->cam_front = vec3_normalize(system->cam_front);

  system->cam_right =
      vec3_normalize(vec3_cross(system->cam_front, (vec3_t){0.0, 1.0, 0.0}));
  system->cam_up =
      vec3_normalize(vec3_cross(system->cam_right, system->cam_front));

  float speed = 10.0f * (float)window->delta_time;
  vec3_t movement = vec3_zero();

  if (re_window_is_scancode_pressed(window, SDL_SCANCODE_W)) {
    movement = vec3_add(movement, system->cam_front);
  }
  if (re_window_is_scancode_pressed(window, SDL_SCANCODE_S)) {
    movement = vec3_sub(movement, system->cam_front);
  }
  if (re_window_is_scancode_pressed(window, SDL_SCANCODE_A)) {
    movement = vec3_sub(movement, system->cam_right);
  }
  if (re_window_is_scancode_pressed(window, SDL_SCANCODE_D)) {
    movement = vec3_add(movement, system->cam_right);
  }

  movement = vec3_muls(movement, speed);

  system->cam_target = vec3_add(system->cam_target, movement);

  camera->position = vec3_lerp(
      camera->position, system->cam_target, (float)window->delta_time * 10.0f);

  camera->rotation =
      quat_conjugate(quat_look_at(system->cam_front, system->cam_up));

  camera->uniform.view =
      mat4_translate(mat4_identity(), vec3_muls(camera->position, -1.0f));
  camera->uniform.view =
      mat4_mul(camera->uniform.view, quat_to_mat4(camera->rotation));

  if (system->transitioning_fov) {
    float progress =
        system->time <= TRANSITION_TIME ? system->time : TRANSITION_TIME;
    progress = remap(progress, 0.0f, TRANSITION_TIME, 0.0, 0.05f);

    camera->fov =
        out_expo(progress, camera->fov, GOAL_FOV - camera->fov, GOAL_FOV);

    if (camera->fov <= GOAL_FOV + 0.001f) {
      system->transitioning_fov = false;
      camera->fov = GOAL_FOV;
    }
  }

  eg_camera_update(camera, window);
}
