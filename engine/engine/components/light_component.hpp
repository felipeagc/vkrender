#pragma once

#include <renderer/glm.hpp>

namespace engine {
struct LightComponent {
  glm::vec3 color{1.0};
  float intensity = 1.0f;
};
} // namespace engine
