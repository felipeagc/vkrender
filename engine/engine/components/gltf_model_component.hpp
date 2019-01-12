#pragma once

#include "../asset_manager.hpp"
#include "../assets/gltf_model_asset.hpp"
#include <renderer/window.hpp>

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
      const re_window_t *window,
      engine::AssetManager &assetManager,
      re_pipeline_t pipeline,
      const glm::mat4 &transform);

  AssetIndex m_modelIndex;

  struct ModelUniform {
    glm::mat4 model{1.0};
  } m_ubo;

  re_resource_set_t m_descriptorSets[renderer::MAX_FRAMES_IN_FLIGHT];

private:
  void *m_mappings[renderer::MAX_FRAMES_IN_FLIGHT];
  re_buffer_t m_uniformBuffers[renderer::MAX_FRAMES_IN_FLIGHT];

  void drawNode(
      GltfModelAsset &model,
      GltfModelAsset::Node &node,
      const re_window_t *window,
      re_pipeline_t pipeline);
};
} // namespace engine
