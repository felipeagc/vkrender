#pragma once

namespace vkr {
class Window;
class Camera;
struct AssetManager;
class LightManager;

namespace imgui {
void statsWindow(Window &window);

void cameraWindow(Camera &camera);

void assetsWindow(AssetManager &assetManager);

void lightsWindow(LightManager &lightManager);
} // namespace imgui
} // namespace vkr
