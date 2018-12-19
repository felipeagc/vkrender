#pragma once

#include <ecs/world.hpp>
#include <renderer/pipeline.hpp>

namespace engine {
class FPSCameraSystem {
public:
  FPSCameraSystem();
  ~FPSCameraSystem();

  void processEvent(renderer::Window &window, SDL_Event event);
  void process(renderer::Window &window, ecs::World &world);

private:
  glm::vec3 m_camUp;
  glm::vec3 m_camFront;
  glm::vec3 m_camRight;
  float m_camYaw = glm::radians(90.0f);
  float m_camPitch = 0.0;

  int m_prevMouseX = 0;
  int m_prevMouseY = 0;
};
} // namespace engine
