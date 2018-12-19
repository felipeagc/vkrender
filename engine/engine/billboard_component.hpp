#pragma once
#include <renderer/buffer.hpp>
#include <renderer/pipeline.hpp>
#include <renderer/texture.hpp>
#include <renderer/window.hpp>

namespace engine {
class BillboardComponent {
public:
  // Creates an initialized Billboard with the given parameters
  BillboardComponent(const renderer::Texture &texture);

  ~BillboardComponent();

  // Billboard can't be copied
  BillboardComponent(const BillboardComponent &) = delete;
  BillboardComponent &operator=(const BillboardComponent &) = delete;

  // Billboard can be moved
  BillboardComponent(BillboardComponent &&rhs) = delete;
  BillboardComponent &operator=(BillboardComponent &&rhs) = delete;

  void draw(
      renderer::Window &window,
      renderer::GraphicsPipeline &pipeline,
      const glm::mat4 &transform,
      const glm::vec3 &color);

protected:
  renderer::Texture m_texture;

  struct BillboardUniform {
    glm::mat4 model = glm::mat4(1.0f);
    glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
  } m_ubo;

  VkDescriptorSet m_materialDescriptorSets[renderer::MAX_FRAMES_IN_FLIGHT];
};
} // namespace engine
