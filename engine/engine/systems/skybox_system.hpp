#pragma once

#include <ecs/world.hpp>
#include <renderer/pipeline.hpp>
#include <renderer/window.hpp>

namespace engine {
class SkyboxSystem {
public:
  SkyboxSystem();
  ~SkyboxSystem();

  void process(
      const re_window_t *window, ecs::World &world, re_pipeline_t &pipeline);
};
} // namespace engine
