#pragma once

#include <ecs/world.hpp>
#include <renderer/pipeline.hpp>
#include <renderer/window.hpp>

namespace engine {
class BillboardSystem {
public:
  BillboardSystem();
  ~BillboardSystem();

  void process(
      renderer::Window &window,
      ecs::World &world,
      renderer::GraphicsPipeline &pipeline);

private:
  bool drawBillboards = true;
};
} // namespace engine
