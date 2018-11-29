#pragma once

#include <ecs/entity.hpp>

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

void cameraWindow(CameraComponent *camera, TransformComponent *transform);

void assetsWindow(AssetManager &assetManager);

void lightSection(
    ecs::Entity entity, TransformComponent &transform, LightComponent &light);
} // namespace imgui
} // namespace engine
