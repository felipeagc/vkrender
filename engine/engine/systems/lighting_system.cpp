#include "lighting_system.hpp"

#include "../components/environment_component.hpp"
#include "../components/light_component.hpp"
#include "../components/transform_component.hpp"

using namespace engine;

LightingSystem::LightingSystem() {}

LightingSystem::~LightingSystem() {}

void LightingSystem::process(renderer::Window &window, ecs::World &world) {
  ecs::Entity environmentEntity = world.first<EnvironmentComponent>();

  auto environment =
      world.getComponent<EnvironmentComponent>(environmentEntity);

  environment->resetLights();

  world.each<TransformComponent, LightComponent>(
      [&](ecs::Entity, TransformComponent &transform, LightComponent &light) {
        environment->addLight(
            transform.position, light.color * light.intensity);
      });

  // Update uniforms
  environment->update(window);
}
