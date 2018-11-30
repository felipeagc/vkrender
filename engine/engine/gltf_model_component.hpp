#pragma once

#include "gltf_model.hpp"

namespace engine {

class GltfModelComponent {
public:
  // Creates an initialized GltfModelInstance with the given model
  GltfModelComponent(const GltfModel &model);

  ~GltfModelComponent();

  // GltfModelInstance cannot be copied
  GltfModelComponent(const GltfModelComponent &) = delete;
  GltfModelComponent &operator=(const GltfModelComponent &) = delete;

  // GltfModelInstance cannot be moved
  GltfModelComponent(GltfModelComponent &&) = delete;
  GltfModelComponent &operator=(GltfModelComponent &&) = delete;

  void draw(
      renderer::Window &window,
      renderer::GraphicsPipeline &pipeline,
      const glm::mat4 &transform);

private:
  GltfModel m_model;

  struct ModelUniform {
    glm::mat4 model{1.0};
  } m_ubo;

  renderer::Buffer m_uniformBuffers[renderer::MAX_FRAMES_IN_FLIGHT];
  void *m_mappings[renderer::MAX_FRAMES_IN_FLIGHT];
  VkDescriptorSet m_descriptorSets[renderer::MAX_FRAMES_IN_FLIGHT];

  // Runs in every draw call to update the uniform transform matrix with
  // updated position, scale and rotations
  void updateUniforms(int frameIndex, const glm::mat4 &transform);

  void drawNode(
      GltfModel::Node &node,
      renderer::Window &window,
      renderer::GraphicsPipeline &pipeline);
};
} // namespace engine
