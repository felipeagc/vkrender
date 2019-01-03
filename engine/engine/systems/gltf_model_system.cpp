#include "gltf_model_system.hpp"

#include "../assets/shape_asset.hpp"
#include "../components/camera_component.hpp"
#include "../components/environment_component.hpp"
#include "../components/gltf_model_component.hpp"
#include "../components/transform_component.hpp"
#include <renderer/window.hpp>

using namespace engine;

GltfModelSystem::GltfModelSystem() {}

GltfModelSystem::~GltfModelSystem() {}

void GltfModelSystem::process(
    renderer::Window &window,
    AssetManager &assetManager,
    ecs::World &world,
    renderer::GraphicsPipeline &modelPipeline) {
  // Draw models
  ecs::Entity camera = world.first<engine::CameraComponent>();
  ecs::Entity skybox = world.first<engine::EnvironmentComponent>();

  world.getComponent<engine::CameraComponent>(camera)->bind(
      window, modelPipeline);

  world.getComponent<engine::EnvironmentComponent>(skybox)->bind(
      window, modelPipeline, 4);

  world.each<engine::GltfModelComponent, engine::TransformComponent>(
      [&](ecs::Entity,
          engine::GltfModelComponent &model,
          engine::TransformComponent &transform) {
        model.draw(window, assetManager, modelPipeline, transform.getMatrix());
      });
}
