#include "imgui_utils.hpp"
#include "asset_manager.hpp"
#include "components/camera_component.hpp"
#include "components/environment_component.hpp"
#include "components/gltf_model_component.hpp"
#include "components/light_component.hpp"
#include "components/transform_component.hpp"
#include <imgui/imgui.h>
#include <renderer/context.hpp>
#include <renderer/util.hpp>
#include <renderer/window.hpp>

namespace engine {
namespace imgui {
void statsWindow(renderer::Window &window) {
  ImGui::Begin("Stats");

  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(renderer::ctx().m_physicalDevice, &properties);

  ImGui::Text("Physical device: %s", properties.deviceName);
  ImGui::Text("Delta time: %.3f s", window.getDelta());
  ImGui::Text("Framerate: %.2f", 1.0f / window.getDelta());

  ImGui::End();
}

void assetsWindow(AssetManager &assetManager) {
  ImGui::Begin("Assets");

  assetManager.each([&](Asset *asset) {
    std::string s = std::to_string(asset->index()) + ": " + asset->identifier();
    ImGui::Selectable(s.c_str(), false);
  });

  ImGui::End();
}
} // namespace imgui
} // namespace engine
