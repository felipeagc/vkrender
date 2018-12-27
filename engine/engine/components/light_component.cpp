#include "light_component.hpp"
#include "../scene.hpp"

using namespace engine;

template <>
void engine::loadComponent<LightComponent>(
    const scene::Component &comp,
    ecs::World &world,
    AssetManager &,
    ecs::Entity entity) {
  glm::vec3 color(1.0);
  float intensity = 1.0f;

  if (comp.properties.find("color") != comp.properties.end()) {
    color = comp.properties.at("color").getVec3();
  }

  if (comp.properties.find("intensity") != comp.properties.end()) {
    intensity = comp.properties.at("intensity").getFloat();
  }

  world.assign<engine::LightComponent>(entity, color, intensity);
}
