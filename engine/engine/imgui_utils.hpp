#pragma once

namespace renderer {
class Window;
}

namespace engine {
class AssetManager;
class LightManager;
struct TransformComponent;
class CameraComponent;

namespace imgui {
void statsWindow(renderer::Window &window);

void cameraWindow(CameraComponent *camera, TransformComponent *transform);

void assetsWindow(AssetManager &assetManager);

void lightsWindow(LightManager &lightManager);
} // namespace imgui
} // namespace engine
