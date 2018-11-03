#pragma once

#include "buffer.hpp"
#include "pipeline.hpp"
#include "window.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace vkr {

class Camera {
public:
  struct CameraUniform {
    glm::mat4 view = glm::mat4();
    glm::mat4 proj = glm::mat4(1.0f);
  };

  Camera(glm::vec3 position = glm::vec3(0.0f));
  ~Camera();

  void setPos(glm::vec3 pos);
  glm::vec3 getPos() const;

  void setFov(float fov);
  float getFov() const;

  void lookAt(glm::vec3 point);

  void update(Window &window);

  void bind(Window &window, GraphicsPipeline &pipeline);

protected:
  std::array<void *, MAX_FRAMES_IN_FLIGHT> mappings;
  std::array<Buffer, MAX_FRAMES_IN_FLIGHT> uniformBuffers;
  std::array<DescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;

  CameraUniform cameraUniform;

  glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f);

  float fov = 70.0f;

  float near = 0.001f;
  float far = 300.0f;
};
} // namespace vkr
