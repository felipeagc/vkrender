#pragma once

#include <ecs/world.hpp>
#include <renderer/window.hpp>

namespace engine {
class LightingSystem {
public:
  LightingSystem();
  ~LightingSystem();

  void process(const re_window_t *window, ecs::World &world);
};
} // namespace engine
