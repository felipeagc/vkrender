#pragma once
#include <renderer/buffer.hpp>
#include <renderer/cubemap.hpp>
#include <renderer/pipeline.hpp>
#include <renderer/texture.hpp>
#include <renderer/window.hpp>

namespace engine {
class SkyboxComponent {
public:
  // Creates an initialized Skybox with the given parameters
  SkyboxComponent(
      const renderer::Cubemap &envCubemap,
      const renderer::Cubemap &irradianceCubemap,
      const renderer::Cubemap &radianceCubemap,
      const renderer::Texture &brdfLut);

  ~SkyboxComponent();

  // Skybox can't be copied
  SkyboxComponent(const SkyboxComponent &) = delete;
  SkyboxComponent &operator=(const SkyboxComponent &) = delete;

  // Skybox can be moved
  SkyboxComponent(SkyboxComponent &&rhs) = delete;
  SkyboxComponent &operator=(SkyboxComponent &&rhs) = delete;

  void bind(
      renderer::Window &window,
      renderer::GraphicsPipeline &pipeline,
      uint32_t setIndex);

  void draw(renderer::Window &window, renderer::GraphicsPipeline &pipeline);

  void update(renderer::Window &window);

  struct EnvironmentUniform {
    float exposure = 1.0;
  } m_environmentUBO;

protected:
  renderer::Cubemap m_envCubemap;
  renderer::Cubemap m_irradianceCubemap;
  renderer::Cubemap m_radianceCubemap;
  renderer::Texture m_brdfLut;

  renderer::Buffer m_uniformBuffers[renderer::MAX_FRAMES_IN_FLIGHT];
  void *m_mappings[renderer::MAX_FRAMES_IN_FLIGHT];
  VkDescriptorSet m_descriptorSets[renderer::MAX_FRAMES_IN_FLIGHT];
};
} // namespace engine
