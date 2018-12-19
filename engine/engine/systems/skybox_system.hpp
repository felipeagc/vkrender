#pragma once

#include <ecs/world.hpp>
#include <renderer/window.hpp>
#include <renderer/pipeline.hpp>

namespace engine {
class SkyboxSystem {
public:
  SkyboxSystem();
  ~SkyboxSystem();

  void process(
      renderer::Window &window,
      ecs::World &world,
      renderer::GraphicsPipeline &pipeline);
};
} // namespace engine
