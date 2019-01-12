#pragma once

#include <ecs/world.hpp>
#include <renderer/pipeline.hpp>

namespace renderer {
class Window;
}

namespace engine {
class SkyboxSystem {
public:
  SkyboxSystem();
  ~SkyboxSystem();

  void process(
      renderer::Window &window,
      ecs::World &world,
      re_pipeline_t &pipeline);
};
} // namespace engine
