#pragma once

#include "../asset_manager.hpp"
#include <ecs/world.hpp>
#include <renderer/pipeline.hpp>

namespace engine {
class GltfModelSystem {
public:
  GltfModelSystem(AssetManager &assetManager);
  ~GltfModelSystem();

  void process(
      renderer::Window &window,
      AssetManager &assetManager,
      ecs::World &world,
      renderer::GraphicsPipeline &pipeline,
      renderer::GraphicsPipeline &boxPipeline);

protected:
  AssetIndex m_boxAsset;
};
} // namespace engine
