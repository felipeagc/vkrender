#pragma once

#include <ecs/world.hpp>
#include <renderer/pipeline.hpp>

namespace engine {
class GltfModelSystem {
public:
  GltfModelSystem();
  ~GltfModelSystem();

  void process(
      renderer::Window &window,
      ecs::World &world,
      renderer::GraphicsPipeline &pipeline);
};
} // namespace engine
