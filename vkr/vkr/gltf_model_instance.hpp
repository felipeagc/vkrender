#pragma once

#include "gltf_model.hpp"

namespace vkr {
class GltfModelInstance {
public:
  GltfModelInstance();
  GltfModelInstance(const GltfModel &model);
  ~GltfModelInstance();

  // GltfModelInstance cannot be copied
  GltfModelInstance(const GltfModelInstance &) = delete;
  GltfModelInstance &operator=(const GltfModelInstance &) = delete;

  // GltfModelInstance can be moved
  GltfModelInstance(GltfModelInstance &&rhs);
  GltfModelInstance &operator=(GltfModelInstance &&rhs);

  void draw(Window &window, GraphicsPipeline &pipeline);

  glm::vec3 m_pos = {0.0, 0.0, 0.0};
  glm::vec3 m_scale = {1.0, 1.0, 1.0};
  glm::vec3 m_rotation = {0.0, 0.0, 0.0};

private:
  GltfModel m_model;

  struct ModelUniform {
    glm::mat4 model{1.0};
  } m_ubo;

  buffer::Buffers<MAX_FRAMES_IN_FLIGHT> m_uniformBuffers;
  void *m_mappings[MAX_FRAMES_IN_FLIGHT];
  VkDescriptorSet m_descriptorSets[MAX_FRAMES_IN_FLIGHT];

  // Runs in every draw call to update the uniform transform matrix with
  // updated position, scale and rotations
  void updateUniforms(int frameIndex);

  void
  drawNode(GltfModel::Node &node, Window &window, GraphicsPipeline &pipeline);
};
} // namespace vkr
