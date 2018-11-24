#pragma once

#include "buffer.hpp"
#include "window.hpp"

namespace vkr {
class GraphicsPipeline;

const uint32_t MAX_LIGHTS = 20;

struct Light {
  glm::vec4 pos;
  glm::vec4 color;
};

class LightManager {
public:
  LightManager(const fstl::fixed_vector<Light> &lights);

  void update();

  void bind(Window &window, GraphicsPipeline &pipeline);

  Light *getLights();

  uint32_t getLightCount() const;
  void setLightCount(uint32_t count);

  // TODO: replace with destructor
  void destroy();

private:
  struct LightingUniform {
    Light lights[MAX_LIGHTS];
    uint32_t lightCount;
  } m_ubo;

  buffer::Buffers<MAX_FRAMES_IN_FLIGHT> m_uniformBuffers;
  void *m_mappings[MAX_FRAMES_IN_FLIGHT];
  VkDescriptorSet m_descriptorSets[MAX_FRAMES_IN_FLIGHT];
};
} // namespace vkr
