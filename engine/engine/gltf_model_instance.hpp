#pragma once

#include "gltf_model.hpp"

namespace engine {
class GltfModelInstance {
public:
  // Creates an uninitialized GltfModelInstance
  GltfModelInstance();

  // Creates an initialized GltfModelInstance with the given model
  GltfModelInstance(const GltfModel &model);

  ~GltfModelInstance();

  // GltfModelInstance cannot be copied
  GltfModelInstance(const GltfModelInstance &) = delete;
  GltfModelInstance &operator=(const GltfModelInstance &) = delete;

  // GltfModelInstance can be moved
  GltfModelInstance(GltfModelInstance &&rhs);
  GltfModelInstance &operator=(GltfModelInstance &&rhs);

  // Returns whether the object is initialized or not
  operator bool() const;

  void draw(renderer::Window &window, renderer::GraphicsPipeline &pipeline);

  glm::vec3 m_pos = {0.0, 0.0, 0.0};
  glm::vec3 m_scale = {1.0, 1.0, 1.0};
  glm::vec3 m_rotation = {0.0, 0.0, 0.0};

private:
  GltfModel m_model;

  struct ModelUniform {
    glm::mat4 model{1.0};
  } m_ubo;

  renderer::buffer::Buffers<renderer::MAX_FRAMES_IN_FLIGHT> m_uniformBuffers;
  void *m_mappings[renderer::MAX_FRAMES_IN_FLIGHT];
  VkDescriptorSet m_descriptorSets[renderer::MAX_FRAMES_IN_FLIGHT];

  // Runs in every draw call to update the uniform transform matrix with
  // updated position, scale and rotations
  void updateUniforms(int frameIndex);

  void drawNode(
      GltfModel::Node &node,
      renderer::Window &window,
      renderer::GraphicsPipeline &pipeline);
};
} // namespace engine
