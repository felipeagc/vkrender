#pragma once

#include "../asset_manager.hpp"
#include <ecs/world.hpp>
#include <renderer/pipeline.hpp>

namespace renderer {
class Window;
}

namespace engine {
class GltfModelSystem {
public:
  GltfModelSystem();
  ~GltfModelSystem();

  void process(
      renderer::Window &window,
      AssetManager &assetManager,
      ecs::World &world,
      re_pipeline_t pipeline);

protected:
  AssetIndex m_boxAsset;
};
} // namespace engine
