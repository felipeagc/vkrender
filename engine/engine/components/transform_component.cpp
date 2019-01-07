#include "transform_component.hpp"
#include "../scene.hpp"

using namespace engine;

template <>
void engine::loadComponent<TransformComponent>(
    const sdf::Component &comp,
    ecs::World &world,
    AssetManager &,
    ecs::Entity entity) {
  glm::vec3 pos(0.0);
  glm::vec3 scale(1.0);
  glm::quat rotation{1.0, 0.0, 0.0, 0.0};

  for (auto &prop : comp.properties) {
    if (strcmp(prop.name, "position") == 0) {
      prop.get_vec3(&pos);
    }

    if (strcmp(prop.name, "scale") == 0) {
      prop.get_vec3(&scale);
    }

    if (strcmp(prop.name, "rotation") == 0) {
      prop.get_quat(&rotation);
    }
  }

  world.assign<engine::TransformComponent>(entity, pos, scale, rotation);
}
