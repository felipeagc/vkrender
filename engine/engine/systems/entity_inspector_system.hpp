#pragma once

#include "../asset_manager.hpp"
#include <ecs/entity.hpp>
#include <renderer/pipeline.hpp>
#include <renderer/window.hpp>

namespace ecs {
class World;
}

namespace engine {
class EntityInspectorSystem {
public:
  EntityInspectorSystem(AssetManager &assetManager);
  ~EntityInspectorSystem();

  void imgui(ecs::World &world);

  void drawBox(
      const re_window_t *window,
      AssetManager &assetManager,
      ecs::World &world,
      re_pipeline_t wireframe_pipeline);

  void processEvent(
      const re_window_t *window,
      AssetManager &assetManager,
      ecs::World &world,
      const SDL_Event &event);

  AssetIndex m_boxAsset;

  ecs::Entity m_selectedEntity = ecs::MAX_ENTITY;
};
} // namespace engine
