#include "fps_camera_system.h"
#include "../camera.h"
#include "../imgui.h"
#include <float.h>
#include <renderer/window.h>

float out_expo(float t, float b, float c, float d) {
  if (t == d) return b + c;
  return c * 1.001f * (-powf(2, -10 * t / d) + 1) + b;
}

void eg_fps_camera_system_init(
    eg_fps_camera_system_t *system, eg_camera_t *camera) {
  system->camera = camera;

  system->cam_target = system->camera->position;

  system->cam_up = (vec3_t){0};
  system->cam_front = (vec3_t){0.0, 0.0, 1.0};
  system->cam_right = (vec3_t){0};

  system->cam_yaw = to_radians(90.0f);
  system->cam_pitch = 0.0f;

  system->prev_normal_cursor_x = 0;
  system->prev_normal_cursor_y = 0;

  system->prev_disabled_cursor_x = 0;
  system->prev_disabled_cursor_y = 0;

  system->prev_right_pressed = false;

  system->sensitivity = 0.07f;

  system->time = 0.0f;
}

void eg_fps_camera_system_update(
    eg_fps_camera_system_t *system,
    re_window_t *window,
    re_cmd_buffer_t *cmd_buffer) {
  float speed = 10.0f * (float)window->delta_time;
  vec3_t movement = vec3_zero();

  system->time += (float)window->delta_time;

  if (!igIsAnyItemActive()) {
    // Camera control toggle
    if (system->prev_right_pressed !=
        re_window_is_mouse_right_pressed(window)) {
      if (re_window_get_input_mode(window, GLFW_CURSOR) ==
          GLFW_CURSOR_DISABLED) {
        re_window_set_input_mode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        re_window_set_cursor_pos(
            window, system->prev_normal_cursor_x, system->prev_normal_cursor_y);
      } else {
        re_window_get_cursor_pos(
            window,
            &system->prev_normal_cursor_x,
            &system->prev_normal_cursor_y);
        re_window_set_input_mode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      }
    }

    system->prev_right_pressed = re_window_is_mouse_right_pressed(window);

    // Camera mouse controls
    double x, y;
    re_window_get_cursor_pos(window, &x, &y);

    double dx = x - system->prev_disabled_cursor_x;
    double dy = y - system->prev_disabled_cursor_y;

    system->prev_disabled_cursor_x = x;
    system->prev_disabled_cursor_y = y;

    if (re_window_is_mouse_right_pressed(window)) {
      if (re_window_get_input_mode(window, GLFW_CURSOR) ==
          GLFW_CURSOR_DISABLED) {
        system->cam_yaw += to_radians((float)dx) * system->sensitivity;
        system->cam_pitch -= to_radians((float)dy) * system->sensitivity;
        system->cam_pitch =
            clamp(system->cam_pitch, to_radians(-89.0f), to_radians(89.0f));
      }
    }

    system->cam_front.x = cosf(system->cam_yaw) * cosf(system->cam_pitch);
    system->cam_front.y = sinf(system->cam_pitch);
    system->cam_front.z = sinf(system->cam_yaw) * cosf(system->cam_pitch);
    system->cam_front = vec3_normalize(system->cam_front);

    system->cam_right =
        vec3_normalize(vec3_cross(system->cam_front, (vec3_t){0.0, 1.0, 0.0}));
    system->cam_up =
        vec3_normalize(vec3_cross(system->cam_right, system->cam_front));

    // Keyboard movement
    if (re_window_is_key_pressed(window, GLFW_KEY_W)) {
      movement = vec3_add(movement, system->cam_front);
    }
    if (re_window_is_key_pressed(window, GLFW_KEY_S)) {
      movement = vec3_sub(movement, system->cam_front);
    }
    if (re_window_is_key_pressed(window, GLFW_KEY_A)) {
      movement = vec3_sub(movement, system->cam_right);
    }
    if (re_window_is_key_pressed(window, GLFW_KEY_D)) {
      movement = vec3_add(movement, system->cam_right);
    }

    movement = vec3_muls(movement, speed);
  }

  system->cam_target = vec3_add(system->cam_target, movement);

  vec3_t lerp = vec3_lerp(
      system->camera->position,
      system->cam_target,
      (float)window->delta_time * 10.0f);

  // If the lerp is large enough or the camera is actively moving
  if (vec3_mag(vec3_sub(lerp, system->camera->position)) >=
          0.02f * (float)window->delta_time ||
      vec3_mag(movement) >= FLT_EPSILON) {
    // Then actually apply the lerp
    system->camera->position = lerp;
  }

  system->camera->rotation =
      quat_conjugate(quat_look_at(system->cam_front, system->cam_up));

  system->camera->uniform.view = mat4_translate(
      mat4_identity(), vec3_muls(system->camera->position, -1.0f));
  system->camera->uniform.view = mat4_mul(
      system->camera->uniform.view, quat_to_mat4(system->camera->rotation));

  eg_camera_update(system->camera, cmd_buffer);
}
