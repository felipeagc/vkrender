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
      renderer::GraphicsPipeline &pipeline);
};
} // namespace engine
