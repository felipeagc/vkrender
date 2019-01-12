#pragma once

#include <ecs/world.hpp>
#include <renderer/pipeline.hpp>

namespace renderer {
class Window;
}

namespace engine {
class BillboardSystem {
public:
  BillboardSystem();
  ~BillboardSystem();

  void process(
      renderer::Window &window,
      ecs::World &world,
      re_pipeline_t &pipeline);

private:
  bool drawBillboards = true;
};
} // namespace engine
