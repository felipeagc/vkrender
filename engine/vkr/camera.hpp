#pragma once

#include "buffer.hpp"
#include "pipeline.hpp"
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

  void update(uint32_t width, uint32_t height);

  void bind(CommandBuffer &commandBuffer, GraphicsPipeline &pipeline);

protected:
  void *mapping;
  Buffer uniformBuffer;
  DescriptorSet descriptorSet;
  DescriptorBufferInfo bufferInfo;

  CameraUniform cameraUniform;

  glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f);

  float fov = 70.0f;

  float near = 0.001f;
  float far = 300.0f;
};
} // namespace vkr
