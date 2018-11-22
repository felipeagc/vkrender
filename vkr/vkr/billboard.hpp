#pragma once
#include "buffer.hpp"
#include "texture.hpp"
#include "window.hpp"

namespace vkr {
struct GraphicsPipeline;

class Billboard {
public:
  Billboard(){};
  Billboard(Texture &texture, glm::vec3 pos, glm::vec3 scale, glm::vec4 color);

  void draw(Window &window, GraphicsPipeline &pipeline);

  void setPos(glm::vec3 pos);
  void setColor(glm::vec4 color);

  void destroy();

protected:
  Texture texture_;

  struct MeshUniform {
    glm::mat4 model;
  } meshUbo_;

  struct MaterialUniform {
    glm::vec4 color;
  } materialUbo_;

  buffer::Buffers<MAX_FRAMES_IN_FLIGHT> meshUniformBuffers_;
  void *meshMappings_[MAX_FRAMES_IN_FLIGHT];
  VkDescriptorSet meshDescriptorSets_[MAX_FRAMES_IN_FLIGHT];

  buffer::Buffers<MAX_FRAMES_IN_FLIGHT> materialUniformBuffers_;
  void *materialMappings_[MAX_FRAMES_IN_FLIGHT];
  VkDescriptorSet materialDescriptorSets_[MAX_FRAMES_IN_FLIGHT];
};
} // namespace vkr
