#pragma once

#include <ecs/entity.hpp>
#include <ecs/world.hpp>

namespace renderer {
class Window;
}

namespace engine {
class AssetManager;
class LightManager;
struct TransformComponent;
struct LightComponent;
class CameraComponent;

namespace imgui {
void statsWindow(renderer::Window &window);

void assetsWindow(AssetManager &assetManager);
} // namespace imgui
} // namespace engine
