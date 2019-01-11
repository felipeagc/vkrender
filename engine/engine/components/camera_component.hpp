#pragma once

#include "transform_component.hpp"
#include <renderer/buffer.hpp>
#include <renderer/common.hpp>
#include <renderer/glm.hpp>
#include <renderer/pipeline.hpp>
#include <renderer/resource_manager.hpp>

namespace renderer {
class Window;
}

namespace engine {
class CameraComponent {
public:
  struct CameraUniform {
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec4 pos;
  };

  // Creates an initialized camera with the given parameters
  CameraComponent(float fov = 70.0f);

  ~CameraComponent();

  // Camera cannot be copied
  CameraComponent(const CameraComponent &) = delete;
  CameraComponent &operator=(const CameraComponent &) = delete;

  // Camera cannot be moved
  CameraComponent(CameraComponent &&) = delete;
  CameraComponent &operator=(CameraComponent &&) = delete;

  // Updates the GPU resources with the data in this Camera object
  void update(renderer::Window &window, const TransformComponent &transform);

  // Binds this camera to a pipeline
  void bind(renderer::Window &window, renderer::GraphicsPipeline &pipeline);

  float m_fov;
  CameraUniform m_cameraUniform;

protected:
  re_buffer_t m_uniformBuffers[renderer::MAX_FRAMES_IN_FLIGHT];
  void *m_mappings[renderer::MAX_FRAMES_IN_FLIGHT];
  renderer::ResourceSet m_descriptorSets[renderer::MAX_FRAMES_IN_FLIGHT];

  float m_near = 0.001f;
  float m_far = 300.0f;
};
} // namespace engine
