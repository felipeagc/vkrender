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
      const re_window_t *window, ecs::World &world, re_pipeline_t &pipeline);

private:
  bool drawBillboards = true;
};
} // namespace engine
