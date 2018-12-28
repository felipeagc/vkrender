#pragma once

#include <ecs/world.hpp>
#include <renderer/pipeline.hpp>

union SDL_Event;

namespace renderer {
class Window;
}

namespace engine {
class FPSCameraSystem {
public:
  FPSCameraSystem();
  ~FPSCameraSystem();

  void processEvent(renderer::Window &window, const SDL_Event &event);
  void process(renderer::Window &window, ecs::World &world);

private:
  glm::vec3 m_camTarget;

  glm::vec3 m_camUp;
  glm::vec3 m_camFront;
  glm::vec3 m_camRight;
  float m_camYaw = glm::radians(90.0f);
  float m_camPitch = 0.0;

  int m_prevMouseX = 0;
  int m_prevMouseY = 0;

  bool m_firstFrame = true;
};
} // namespace engine
