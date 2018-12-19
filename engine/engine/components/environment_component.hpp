#pragma once
#include <renderer/buffer.hpp>
#include <renderer/cubemap.hpp>
#include <renderer/pipeline.hpp>
#include <renderer/texture.hpp>
#include <renderer/window.hpp>

namespace engine {
const uint32_t MAX_LIGHTS = 20;

class EnvironmentComponent {
public:
  // Creates an initialized Skybox with the given parameters
  EnvironmentComponent(
      const renderer::Cubemap &envCubemap,
      const renderer::Cubemap &irradianceCubemap,
      const renderer::Cubemap &radianceCubemap,
      const renderer::Texture &brdfLut);

  ~EnvironmentComponent();

  // Environment can't be copied
  EnvironmentComponent(const EnvironmentComponent &) = delete;
  EnvironmentComponent &operator=(const EnvironmentComponent &) = delete;

  // Environment can't be moved
  EnvironmentComponent(EnvironmentComponent &&rhs) = delete;
  EnvironmentComponent &operator=(EnvironmentComponent &&rhs) = delete;

  void bind(
      renderer::Window &window,
      renderer::GraphicsPipeline &pipeline,
      uint32_t setIndex);

  void
  drawSkybox(renderer::Window &window, renderer::GraphicsPipeline &pipeline);

  void update(renderer::Window &window);

  void setExposure(float exposure);
  float getExposure();

  void addLight(const glm::vec3 &pos, const glm::vec3 &color);
  void resetLights();

protected:
  struct Light {
    glm::vec4 pos;
    glm::vec4 color;
  };

  struct EnvironmentUniform {
    Light lights[MAX_LIGHTS];
    float exposure = 8.0;
    uint lightCount = 0.0;
  } m_ubo;

  renderer::Cubemap m_envCubemap;
  renderer::Cubemap m_irradianceCubemap;
  renderer::Cubemap m_radianceCubemap;
  renderer::Texture m_brdfLut;

  renderer::Buffer m_uniformBuffers[renderer::MAX_FRAMES_IN_FLIGHT];
  void *m_mappings[renderer::MAX_FRAMES_IN_FLIGHT];
  VkDescriptorSet m_descriptorSets[renderer::MAX_FRAMES_IN_FLIGHT];
};
} // namespace engine
