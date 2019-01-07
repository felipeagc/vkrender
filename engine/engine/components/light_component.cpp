#include "light_component.hpp"
#include "../scene.hpp"

using namespace engine;

template <>
void engine::loadComponent<LightComponent>(
    const sdf::Component &comp,
    ecs::World &world,
    AssetManager &,
    ecs::Entity entity) {
  glm::vec3 color(1.0);
  float intensity = 1.0f;

  for (auto &prop : comp.properties) {
    if (strcmp(prop.name, "color") == 0) {
      prop.get_vec3(&color);
    } else if (strcmp(prop.name, "intensity") == 0) {
      prop.get_float(&intensity);
    }
  }

  world.assign<engine::LightComponent>(entity, color, intensity);
}
