#pragma once

#include "gltf_model.hpp"

namespace vkr {
struct GltfModelInstance {
  GltfModel model;

  glm::vec3 pos = {0.0, 0.0, 0.0};
  glm::vec3 scale = {1.0, 1.0, 1.0};
  glm::vec3 rotation = {0.0, 0.0, 0.0};

  GltfModelInstance(const GltfModel &model);
  ~GltfModelInstance(){};

  // TODO: prohibit copies
  GltfModelInstance();

  // TODO: move constructors

  void destroy();

  void draw(Window &window, GraphicsPipeline &pipeline);

private:
  struct ModelUniform {
    glm::mat4 model{1.0};
  } ubo;

  buffer::Buffers<MAX_FRAMES_IN_FLIGHT> uniformBuffers;
  std::array<void *, MAX_FRAMES_IN_FLIGHT> mappings;
  std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;

  // Run every frame to update the uniform transform matrix with
  // updated position, scale and rotations
  void updateUniforms(int frameIndex);

  void
  drawNode(GltfModel::Node &node, Window &window, GraphicsPipeline &pipeline);
};
} // namespace vkr
