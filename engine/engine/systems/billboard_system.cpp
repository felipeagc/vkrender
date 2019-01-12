#include "billboard_system.hpp"

#include "../components/billboard_component.hpp"
#include "../components/camera_component.hpp"
#include "../components/light_component.hpp"
#include <ftl/vector.hpp>

using namespace engine;

BillboardSystem::BillboardSystem() {}

BillboardSystem::~BillboardSystem() {}

void BillboardSystem::process(
    const re_window_t *window, ecs::World &world, re_pipeline_t &pipeline) {
  if (!drawBillboards)
    return;

  ecs::Entity cameraEntity =
      world.first<engine::CameraComponent, engine::TransformComponent>();
  auto camera = world.getComponent<engine::CameraComponent>(cameraEntity);
  auto cameraTransform =
      world.getComponent<engine::TransformComponent>(cameraEntity);

  // Bind camera
  camera->bind(window, pipeline);

  glm::vec3 cameraPos = cameraTransform->position;
  ftl::small_vector<std::pair<float, ecs::Entity>> billboards;

  world.each<
      engine::TransformComponent,
      engine::LightComponent,
      engine::BillboardComponent>([&](ecs::Entity entity,
                                      engine::TransformComponent &transform,
                                      engine::LightComponent &,
                                      engine::BillboardComponent &) {
    billboards.push_back(
        {-glm::distance(cameraPos, transform.position), entity});
  });

  // Sort draw calls
  std::sort(billboards.begin(), billboards.end());

  for (auto &[dist, entity] : billboards) {
    auto transform = world.getComponent<engine::TransformComponent>(entity);
    auto light = world.getComponent<engine::LightComponent>(entity);
    auto billboard = world.getComponent<engine::BillboardComponent>(entity);
    billboard->draw(window, pipeline, transform->getMatrix(), light->color);
  }
}
