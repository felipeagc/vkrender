#include "fps_camera_system.hpp"

#include "../components/camera_component.hpp"
#include "../components/transform_component.hpp"
#include <imgui/imgui.h>
#include <renderer/window.hpp>

using namespace engine;

const float SENSITIVITY = 0.07;

static inline glm::vec3 lerp(glm::vec3 v1, glm::vec3 v2, float t) {
  return v1 + t * (v2 - v1);
}

FPSCameraSystem::FPSCameraSystem() {}

FPSCameraSystem::~FPSCameraSystem() {}

void FPSCameraSystem::processEvent(
    re_window_t *window, const SDL_Event &event) {
  switch (event.type) {
  case SDL_MOUSEBUTTONDOWN:
    if (event.button.button == SDL_BUTTON_RIGHT && !ImGui::IsAnyItemActive()) {
      re_window_get_mouse_state(window, &m_prevMouseX, &m_prevMouseY);
      re_window_set_relative_mouse(window, true);
    }
    break;
  case SDL_MOUSEBUTTONUP:
    if (event.button.button == SDL_BUTTON_RIGHT && !ImGui::IsAnyItemActive()) {
      re_window_set_relative_mouse(window, true);
      re_window_warp_mouse(window, m_prevMouseX, m_prevMouseY);
    }
    break;
  case SDL_MOUSEMOTION:
    if (event.motion.state & SDL_BUTTON_RMASK) {
      int dx = event.motion.xrel;
      int dy = event.motion.yrel;

      if (re_window_get_relative_mouse(window)) {
        m_camYaw += glm::radians((float)dx) * SENSITIVITY;
        m_camPitch -= glm::radians((float)dy) * SENSITIVITY;
        m_camPitch =
            glm::clamp(m_camPitch, glm::radians(-89.0f), glm::radians(89.0f));
      }
    }
    break;
  }
}

void FPSCameraSystem::process(const re_window_t *window, ecs::World &world) {
  ecs::Entity cameraEntity =
      world.first<engine::CameraComponent, engine::TransformComponent>();

  auto camera = world.getComponent<engine::CameraComponent>(cameraEntity);
  auto transform = world.getComponent<engine::TransformComponent>(cameraEntity);

  if (m_firstFrame) {
    m_firstFrame = false;
    m_camTarget = transform->position;
  }

  m_camFront.x = cos(m_camYaw) * cos(m_camPitch);
  m_camFront.y = sin(m_camPitch);
  m_camFront.z = sin(m_camYaw) * cos(m_camPitch);
  m_camFront = glm::normalize(m_camFront);

  m_camRight = glm::normalize(glm::cross(m_camFront, glm::vec3(0.0, 1.0, 0.0)));
  m_camUp = glm::normalize(glm::cross(m_camRight, m_camFront));

  float speed = 10.0f * window->delta_time;
  glm::vec3 movement(0.0);

  if (re_window_is_scancode_pressed(window, SDL_SCANCODE_W))
    movement += m_camFront;
  if (re_window_is_scancode_pressed(window, SDL_SCANCODE_S))
    movement -= m_camFront;
  if (re_window_is_scancode_pressed(window, SDL_SCANCODE_A))
    movement -= m_camRight;
  if (re_window_is_scancode_pressed(window, SDL_SCANCODE_D))
    movement += m_camRight;

  movement *= speed;

  m_camTarget += movement;

  transform->position =
      lerp(transform->position, m_camTarget, window->delta_time * 10.0f);

  transform->lookAt(m_camFront, m_camUp);

  camera->update(window, *transform);
}
