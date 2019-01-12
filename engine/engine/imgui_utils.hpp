#pragma once

#include <ecs/entity.hpp>
#include <ecs/world.hpp>
#include <renderer/window.hpp>

namespace engine {
class AssetManager;
class LightManager;
struct TransformComponent;
struct LightComponent;
class CameraComponent;

namespace imgui {
void statsWindow(const re_window_t *window);

void assetsWindow(AssetManager &assetManager);
} // namespace imgui
} // namespace engine
