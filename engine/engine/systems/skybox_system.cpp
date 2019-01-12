#include "skybox_system.hpp"

#include "../components/camera_component.hpp"
#include "../components/environment_component.hpp"

using namespace engine;

SkyboxSystem::SkyboxSystem() {}

SkyboxSystem::~SkyboxSystem() {}

void SkyboxSystem::process(
    const re_window_t *window,
    ecs::World &world,
    re_pipeline_t &pipeline) {
  ecs::Entity cameraEntity = world.first<CameraComponent>();
  world.getComponent<CameraComponent>(cameraEntity)->bind(window, pipeline);

  ecs::Entity environmentEntity = world.first<EnvironmentComponent>();
  auto environment = world.getComponent<EnvironmentComponent>(environmentEntity);

  environment->drawSkybox(window, pipeline);
}
