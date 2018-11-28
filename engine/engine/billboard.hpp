#pragma once
#include <renderer/buffer.hpp>
#include <renderer/pipeline.hpp>
#include <renderer/texture.hpp>
#include <renderer/window.hpp>

namespace engine {
class Billboard {
public:
  // Creates an uninitialized Billboard
  Billboard();

  // Creates an initialized Billboard with the given parameters
  Billboard(
      const renderer::Texture &texture,
      glm::vec3 pos,
      glm::vec3 scale,
      glm::vec4 color);

  ~Billboard();

  // Billboard can't be copied
  Billboard(const Billboard &) = delete;
  Billboard &operator=(const Billboard &) = delete;

  // Billboard can be moved
  Billboard(Billboard &&rhs);
  Billboard &operator=(Billboard &&rhs);

  // Returns whether the object is initialized or not
  operator bool() const;

  void draw(renderer::Window &window, renderer::GraphicsPipeline &pipeline);

  void setPos(glm::vec3 pos);
  void setColor(glm::vec4 color);

protected:
  renderer::Texture m_texture;

  struct MeshUniform {
    glm::mat4 model;
  } m_meshUbo;

  struct MaterialUniform {
    glm::vec4 color;
  } m_materialUbo;

  renderer::buffer::Buffers<renderer::MAX_FRAMES_IN_FLIGHT>
      m_meshUniformBuffers;
  void *m_meshMappings[renderer::MAX_FRAMES_IN_FLIGHT];
  VkDescriptorSet m_meshDescriptorSets[renderer::MAX_FRAMES_IN_FLIGHT];

  renderer::buffer::Buffers<renderer::MAX_FRAMES_IN_FLIGHT>
      m_materialUniformBuffers;
  void *m_materialMappings[renderer::MAX_FRAMES_IN_FLIGHT];
  VkDescriptorSet m_materialDescriptorSets[renderer::MAX_FRAMES_IN_FLIGHT];
};
} // namespace engine
