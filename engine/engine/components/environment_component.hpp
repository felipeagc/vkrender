#pragma once

#include "../asset_manager.hpp"
#include "../assets/environment_asset.hpp"
#include <renderer/buffer.hpp>
#include <renderer/common.hpp>
#include <renderer/cubemap.hpp>
#include <renderer/pipeline.hpp>
#include <renderer/resource_manager.hpp>
#include <renderer/texture.hpp>
#include <renderer/window.hpp>

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
      const re_window_t *window,
      re_pipeline_t &pipeline,
      uint32_t setIndex);

  void
  drawSkybox(const re_window_t *window, re_pipeline_t &pipeline);

  void update(const re_window_t *window);

  void addLight(const glm::vec3 &pos, const glm::vec3 &color);
  void resetLights();

  struct Light {
    glm::vec4 pos;
    glm::vec4 color;
  };

  struct EnvironmentUniform {
    glm::vec3 sunDirection{0.0, -1.0, 0.0};
    float exposure = 8.0f;
    glm::vec3 sunColor{1.0};
    float sunIntensity = 1.0f;
    float radianceMipLevels = 1.0f;
    uint32_t lightCount = 0;
    alignas(16) Light lights[MAX_LIGHTS];
  } m_ubo;

protected:
  AssetIndex m_environmentAssetIndex;

  re_buffer_t m_uniformBuffers[renderer::MAX_FRAMES_IN_FLIGHT];
  void *m_mappings[renderer::MAX_FRAMES_IN_FLIGHT];
  re_resource_set_t m_descriptorSets[renderer::MAX_FRAMES_IN_FLIGHT];
};
} // namespace engine
