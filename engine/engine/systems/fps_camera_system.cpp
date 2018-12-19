#include "fps_camera_system.hpp"

#include "../components/camera_component.hpp"
#include "../components/transform_component.hpp"
#include <imgui/imgui.h>

using namespace engine;

const float SENSITIVITY = 0.07;

static inline glm::vec3 lerp(glm::vec3 v1, glm::vec3 v2, float t) {
  return v1 + t * (v2 - v1);
}

FPSCameraSystem::FPSCameraSystem() {}

FPSCameraSystem::~FPSCameraSystem() {}

void FPSCameraSystem::processEvent(renderer::Window &window, SDL_Event event) {
  switch (event.type) {
  case SDL_MOUSEBUTTONDOWN:
    if (event.button.button == SDL_BUTTON_RIGHT && !ImGui::IsAnyItemActive()) {
      window.getMouseState(&m_prevMouseX, &m_prevMouseY);
      window.setRelativeMouse(true);
    }
    break;
  case SDL_MOUSEBUTTONUP:
    if (event.button.button == SDL_BUTTON_RIGHT && !ImGui::IsAnyItemActive()) {
      window.setRelativeMouse(false);
      window.warpMouse(m_prevMouseX, m_prevMouseY);
    }
    break;
  case SDL_MOUSEMOTION:
    if (event.motion.state & SDL_BUTTON_RMASK) {
      int dx = event.motion.xrel;
      int dy = event.motion.yrel;

      if (window.getRelativeMouse()) {
        m_camYaw += glm::radians((float)dx) * SENSITIVITY;
        m_camPitch -= glm::radians((float)dy) * SENSITIVITY;
        m_camPitch =
            glm::clamp(m_camPitch, glm::radians(-89.0f), glm::radians(89.0f));
      }
    }
    break;
  }
}

void FPSCameraSystem::process(renderer::Window &window, ecs::World &world) {
  ecs::Entity cameraEntity =
      world.first<engine::CameraComponent, engine::TransformComponent>();

  auto camera = world.getComponent<engine::CameraComponent>(cameraEntity);
  auto transform = world.getComponent<engine::TransformComponent>(cameraEntity);

  static glm::vec3 cameraTarget;

  m_camFront.x = cos(m_camYaw) * cos(m_camPitch);
  m_camFront.y = sin(m_camPitch);
  m_camFront.z = sin(m_camYaw) * cos(m_camPitch);
  m_camFront = glm::normalize(m_camFront);

  m_camRight = glm::normalize(glm::cross(m_camFront, glm::vec3(0.0, 1.0, 0.0)));
  m_camUp = glm::normalize(glm::cross(m_camRight, m_camFront));

  float speed = 10.0f * window.getDelta();
  glm::vec3 movement(0.0);

  if (window.isScancodePressed(renderer::Scancode::eW))
    movement += m_camFront;
  if (window.isScancodePressed(renderer::Scancode::eS))
    movement -= m_camFront;
  if (window.isScancodePressed(renderer::Scancode::eA))
    movement -= m_camRight;
  if (window.isScancodePressed(renderer::Scancode::eD))
    movement += m_camRight;

  movement *= speed;

  cameraTarget += movement;

  transform->position =
      lerp(transform->position, cameraTarget, window.getDelta() * 10.0f);

  transform->lookAt(m_camFront, m_camUp);

  camera->update(window, *transform);
}
