#pragma once

#include <ecs/world.hpp>

namespace renderer {
class Window;
}

namespace engine {
class LightingSystem {
public:
  LightingSystem();
  ~LightingSystem();

  void process(renderer::Window &window, ecs::World &world);
};
} // namespace engine
