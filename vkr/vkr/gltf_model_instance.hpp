#pragma once

#include "gltf_model.hpp"

namespace vkr {
class GltfModelInstance {
public:
  glm::vec3 m_pos = {0.0, 0.0, 0.0};
  glm::vec3 m_scale = {1.0, 1.0, 1.0};
  glm::vec3 m_rotation = {0.0, 0.0, 0.0};

  GltfModelInstance() {}
  GltfModelInstance(const GltfModel &model);
  ~GltfModelInstance(){};

  void destroy();

  void draw(Window &window, GraphicsPipeline &pipeline);

private:
  GltfModel m_model;

  struct ModelUniform {
    glm::mat4 model{1.0};
  } m_ubo;

  buffer::Buffers<MAX_FRAMES_IN_FLIGHT> m_uniformBuffers;
  std::array<void *, MAX_FRAMES_IN_FLIGHT> m_mappings;
  std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_descriptorSets;

  // Run every frame to update the uniform transform matrix with
  // updated position, scale and rotations
  void updateUniforms(int frameIndex);

  void
  drawNode(GltfModel::Node &node, Window &window, GraphicsPipeline &pipeline);
};
} // namespace vkr
