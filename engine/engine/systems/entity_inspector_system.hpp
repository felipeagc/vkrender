#pragma once

#include "../asset_manager.hpp"
#include <ecs/entity.hpp>

union SDL_Event;

namespace ecs {
class World;
}

namespace renderer {
class Window;
struct GraphicsPipeline;
} // namespace renderer

namespace engine {
class EntityInspectorSystem {
public:
  EntityInspectorSystem(AssetManager &assetManager);
  ~EntityInspectorSystem();

  void imgui(ecs::World &world);

  void drawBox(
      renderer::Window &window,
      AssetManager &assetManager,
      ecs::World &world,
      renderer::GraphicsPipeline &boxPipeline);

  void processEvent(
      const renderer::Window &window,
      AssetManager &assetManager,
      ecs::World &world,
      const SDL_Event &event);

  AssetIndex m_boxAsset;

  ecs::Entity m_selectedEntity = ecs::MAX_ENTITY;
};
} // namespace engine
