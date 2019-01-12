#pragma once

#include <ecs/world.hpp>
#include <renderer/pipeline.hpp>
#include <renderer/window.hpp>

namespace engine {
class FPSCameraSystem {
public:
  FPSCameraSystem();
  ~FPSCameraSystem();

  void processEvent(re_window_t *window, const SDL_Event &event);
  void process(const re_window_t* window, ecs::World &world);

private:
  glm::vec3 m_camTarget;

  glm::vec3 m_camUp;
  glm::vec3 m_camFront{0.0, 0.0, 1.0};
  glm::vec3 m_camRight;
  float m_camYaw = glm::radians(90.0f);
  float m_camPitch = 0.0;

  int m_prevMouseX = 0;
  int m_prevMouseY = 0;

  bool m_firstFrame = true;
};
} // namespace engine
