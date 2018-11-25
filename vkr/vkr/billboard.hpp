#pragma once
#include "buffer.hpp"
#include "texture.hpp"
#include "window.hpp"

namespace vkr {
class GraphicsPipeline;

class Billboard {
public:
  // Creates an uninitialized Billboard
  Billboard();

  // Creates an initialized Billboard with the given parameters
  Billboard(
      const Texture &texture, glm::vec3 pos, glm::vec3 scale, glm::vec4 color);

  ~Billboard();

  // Billboard can't be copied
  Billboard(const Billboard &) = delete;
  Billboard &operator=(const Billboard &) = delete;

  // Billboard can be moved
  Billboard(Billboard &&rhs);
  Billboard &operator=(Billboard &&rhs);

  // Returns whether the object is initialized or not
  operator bool() const;

  void draw(Window &window, GraphicsPipeline &pipeline);

  void setPos(glm::vec3 pos);
  void setColor(glm::vec4 color);

protected:
  Texture m_texture;

  struct MeshUniform {
    glm::mat4 model;
  } m_meshUbo;

  struct MaterialUniform {
    glm::vec4 color;
  } m_materialUbo;

  buffer::Buffers<MAX_FRAMES_IN_FLIGHT> m_meshUniformBuffers;
  void *m_meshMappings[MAX_FRAMES_IN_FLIGHT];
  VkDescriptorSet m_meshDescriptorSets[MAX_FRAMES_IN_FLIGHT];

  buffer::Buffers<MAX_FRAMES_IN_FLIGHT> m_materialUniformBuffers;
  void *m_materialMappings[MAX_FRAMES_IN_FLIGHT];
  VkDescriptorSet m_materialDescriptorSets[MAX_FRAMES_IN_FLIGHT];
};
} // namespace vkr
