#pragma once

#include "../asset_manager.hpp"
#include "../assets/environment_asset.hpp"
#include <renderer/buffer.hpp>
#include <renderer/common.hpp>
#include <renderer/cubemap.hpp>
#include <renderer/pipeline.hpp>
#include <renderer/texture.hpp>

namespace renderer {
class Window;
}

namespace engine {
const uint32_t MAX_LIGHTS = 20;

class EnvironmentComponent {
public:
  // Creates an initialized Skybox with the given parameters
  EnvironmentComponent(const EnvironmentAsset &environmentAsset);

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
    float exposure = 8.0;
    uint32_t lightCount = 0;
    alignas(16) Light lights[MAX_LIGHTS];
  } m_ubo;

  AssetIndex m_environmentAssetIndex;

  renderer::Buffer m_uniformBuffers[renderer::MAX_FRAMES_IN_FLIGHT];
  void *m_mappings[renderer::MAX_FRAMES_IN_FLIGHT];
  VkDescriptorSet m_descriptorSets[renderer::MAX_FRAMES_IN_FLIGHT];
};
} // namespace engine
