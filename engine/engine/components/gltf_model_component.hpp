#pragma once

#include "../asset_manager.hpp"
#include "../assets/gltf_model_asset.hpp"

namespace engine {

class GltfModelComponent {
public:
  // Creates an initialized GltfModelInstance with the given model
  GltfModelComponent(const GltfModelAsset &modelAsset);

  ~GltfModelComponent();

  // GltfModelInstance cannot be copied
  GltfModelComponent(const GltfModelComponent &) = delete;
  GltfModelComponent &operator=(const GltfModelComponent &) = delete;

  // GltfModelInstance cannot be moved
  GltfModelComponent(GltfModelComponent &&) = delete;
  GltfModelComponent &operator=(GltfModelComponent &&) = delete;

  void draw(
      renderer::Window &window,
      engine::AssetManager &assetManager,
      renderer::GraphicsPipeline &pipeline,
      const glm::mat4 &transform);

  AssetIndex m_modelIndex;

  struct ModelUniform {
    glm::mat4 model{1.0};
  } m_ubo;

  renderer::ResourceSet m_descriptorSets[renderer::MAX_FRAMES_IN_FLIGHT];

private:
  void *m_mappings[renderer::MAX_FRAMES_IN_FLIGHT];
  renderer::Buffer m_uniformBuffers[renderer::MAX_FRAMES_IN_FLIGHT];

  void drawNode(
      GltfModelAsset &model,
      GltfModelAsset::Node &node,
      renderer::Window &window,
      renderer::GraphicsPipeline &pipeline);
};
} // namespace engine
