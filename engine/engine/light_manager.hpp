#pragma once

#include <renderer/buffer.hpp>
#include <renderer/glm.hpp>
#include <renderer/pipeline.hpp>
#include <renderer/window.hpp>

namespace engine {
const uint32_t MAX_LIGHTS = 20;

struct Light {
  glm::vec4 pos;
  glm::vec4 color;
};

class LightManager {
public:
  LightManager(const fstl::fixed_vector<Light> &lights);
  ~LightManager();

  // LightManager cannot be copied
  LightManager(const LightManager &) = delete;
  LightManager &operator=(const LightManager &) = delete;

  // LightManager cannot be moved
  LightManager(LightManager &&rhs) = delete;
  LightManager &operator=(LightManager &&rhs) = delete;

  void update();

  void bind(renderer::Window &window, renderer::GraphicsPipeline &pipeline);

  Light *getLights();

  uint32_t getLightCount() const;
  void setLightCount(uint32_t count);

private:
  struct LightingUniform {
    Light lights[MAX_LIGHTS];
    uint32_t lightCount;
  } m_ubo;

  renderer::buffer::Buffers<renderer::MAX_FRAMES_IN_FLIGHT> m_uniformBuffers;
  void *m_mappings[renderer::MAX_FRAMES_IN_FLIGHT];
  VkDescriptorSet m_descriptorSets[renderer::MAX_FRAMES_IN_FLIGHT];
};
} // namespace engine
