#include "transform_component.hpp"
#include "../scene.hpp"

using namespace engine;

template <>
void engine::loadComponent<TransformComponent>(
    const scene::Component &comp,
    ecs::World &world,
    AssetManager &,
    ecs::Entity entity) {
  glm::vec3 pos(0.0);
  glm::vec3 scale(1.0);
  glm::quat rotation{1.0, 0.0, 0.0, 0.0};

  if (comp.properties.find("position") != comp.properties.end()) {
    pos = comp.properties.at("position").getVec3();
  }

  if (comp.properties.find("scale") != comp.properties.end()) {
    scale = comp.properties.at("scale").getVec3();
  }

  if (comp.properties.find("rotation") != comp.properties.end()) {
    rotation = comp.properties.at("rotation").getQuat();
  }

  world.assign<engine::TransformComponent>(entity, pos, scale, rotation);
}
