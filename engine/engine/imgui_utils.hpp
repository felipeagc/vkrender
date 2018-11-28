#pragma once

namespace renderer {
class Window;
}

namespace engine {
class Camera;
class AssetManager;
class LightManager;

namespace imgui {
void statsWindow(renderer::Window &window);

void cameraWindow(Camera &camera);

void assetsWindow(AssetManager &assetManager);

void lightsWindow(LightManager &lightManager);
} // namespace imgui
} // namespace engine
