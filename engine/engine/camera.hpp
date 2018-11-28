#pragma once

#include <renderer/buffer.hpp>
#include <renderer/glm.hpp>
#include <renderer/pipeline.hpp>
#include <renderer/window.hpp>

namespace engine {
class Camera {
public:
  struct CameraUniform {
    glm::mat4 view;
    glm::mat4 proj;
  };

  // Creates an uninitialized Camera
  Camera();

  // Creates an initialized camera with the given parameters
  Camera(glm::vec3 position, glm::quat rotation = glm::quat());

  ~Camera();

  // Camera cannot be copied
  Camera(const Camera &) = delete;
  Camera &operator=(const Camera &) = delete;

  // Camera can be moved
  Camera(Camera &&rhs);
  Camera &operator=(Camera &&rhs);

  // Returns whether the object is initialized or not
  operator bool() const;

  void lookAt(glm::vec3 point, glm::vec3 up);

  // Updates the GPU resources with the data in this Camera object
  void update(renderer::Window &window);

  // Binds this camera to a pipeline
  void bind(renderer::Window &window, renderer::GraphicsPipeline &pipeline);

  glm::vec3 m_position;
  glm::quat m_rotation;

protected:
  renderer::buffer::Buffers<renderer::MAX_FRAMES_IN_FLIGHT> m_uniformBuffers;
  void *m_mappings[renderer::MAX_FRAMES_IN_FLIGHT];
  VkDescriptorSet m_descriptorSets[renderer::MAX_FRAMES_IN_FLIGHT];

  CameraUniform m_cameraUniform;

  float m_fov = 70.0f;

  float m_near = 0.001f;
  float m_far = 300.0f;
};
} // namespace engine
