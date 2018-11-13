#pragma once

#include "buffer.hpp"
#include "window.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace vkr {
class GraphicsPipeline;

class Camera {
public:
  struct CameraUniform {
    glm::mat4 view{1.0f};
    glm::mat4 proj{1.0f};
  };

  Camera(
      glm::vec3 position = glm::vec3(0.0f),
      glm::quat rotation = glm::quat(glm::vec3(0.0f, M_PI, M_PI)));
  ~Camera();

  void setPos(glm::vec3 pos);
  glm::vec3 getPos() const;
  void translate(glm::vec3 translation);

  void setRot(glm::quat rot);
  glm::quat getRot() const;
  void rotate(glm::quat rot);

  void setFov(float fov);
  float getFov() const;

  void lookAt(glm::vec3 point);

  void update(Window &window);

  void bind(Window &window, GraphicsPipeline &pipeline);

protected:
  std::array<void *, MAX_FRAMES_IN_FLIGHT> mappings_;
  std::array<Buffer, MAX_FRAMES_IN_FLIGHT> uniformBuffers_;
  std::array<DescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets_;

  CameraUniform cameraUniform_;

  float fov_ = 70.0f;

  float near_ = 0.001f;
  float far_ = 300.0f;
};
} // namespace vkr