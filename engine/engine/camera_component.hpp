#pragma once

#include <renderer/buffer.hpp>
#include <renderer/glm.hpp>
#include <renderer/pipeline.hpp>
#include <renderer/window.hpp>

namespace engine {
class CameraComponent {
public:
  struct CameraUniform {
    glm::mat4 view;
    glm::mat4 proj;
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
  void update(renderer::Window &window, const glm::mat4 &transform);

  // Binds this camera to a pipeline
  void bind(renderer::Window &window, renderer::GraphicsPipeline &pipeline);

  float m_fov;

protected:
  renderer::buffer::Buffers<renderer::MAX_FRAMES_IN_FLIGHT> m_uniformBuffers;
  void *m_mappings[renderer::MAX_FRAMES_IN_FLIGHT];
  VkDescriptorSet m_descriptorSets[renderer::MAX_FRAMES_IN_FLIGHT];

  CameraUniform m_cameraUniform;

  float m_near = 0.001f;
  float m_far = 300.0f;
};
} // namespace engine