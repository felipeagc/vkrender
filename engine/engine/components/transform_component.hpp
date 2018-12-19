#pragma once

#include <renderer/glm.hpp>

namespace engine {
struct TransformComponent {
  glm::vec3 position;
  glm::vec3 scale;
  glm::quat rotation;

  TransformComponent(
      glm::vec3 position = {0.0, 0.0, 0.0},
      glm::vec3 scale = {1.0, 1.0, 1.0},
      glm::quat rotation = {})
      : position(position), scale(scale), rotation(rotation) {}

  inline glm::mat4 getMatrix() const {
    glm::mat4 mat = glm::translate(glm::mat4(1.0f), this->position) *
                    glm::mat4_cast(this->rotation) *
                    glm::scale(glm::mat4(1.0f), this->scale);

    return mat;
  }

  void lookAt(glm::vec3 direction, glm::vec3 up) {
    this->rotation =
        glm::conjugate(glm::quatLookAt(glm::normalize(direction), up));
  }
};
} // namespace engine
