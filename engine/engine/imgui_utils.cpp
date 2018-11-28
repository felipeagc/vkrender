#include "imgui_utils.hpp"
#include "asset_manager.hpp"
#include "camera.hpp"
#include "light_manager.hpp"
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

void cameraWindow(Camera &camera) {
  ImGui::Begin("Camera");

  float camPos[] = {
      camera.m_position.x, camera.m_position.y, camera.m_position.z};
  ImGui::DragFloat3("Camera position", camPos, -10.0f, 10.0f);

  glm::vec3 euler = glm::degrees(glm::eulerAngles(camera.m_rotation));
  float camRot[] = {euler.x, euler.y, euler.z};
  ImGui::DragFloat3("Camera rotation", camRot, -180.0f, 180.0f);

  ImGui::End();
}

void assetsWindow(AssetManager &assetManager) {
  ImGui::Begin("Assets");

  for (auto &[name, asset] : assetManager.getAssetTable()) {
    ImGui::Selectable(name.c_str(), false);
  }

  ImGui::End();
}

void lightsWindow(LightManager &lightManager) {
  ImGui::Begin("Lights");

  for (uint32_t i = 0; i < lightManager.getLightCount(); i++) {
    ImGui::PushID(i);
    ImGui::Text("Light %d", i);

    Light *light = &lightManager.getLights()[i];

    float pos[3] = {
        light->pos.x,
        light->pos.y,
        light->pos.z,
    };
    ImGui::DragFloat3("Position", pos, 1.0f, -10.0f, 10.0f);
    light->pos = {pos[0], pos[1], pos[2], 1.0f};

    float color[4] = {
        light->color.x,
        light->color.y,
        light->color.z,
        light->color.w,
    };
    ImGui::ColorEdit4("Color", color);
    light->color = {color[0], color[1], color[2], color[3]};

    ImGui::Separator();
    ImGui::PopID();
  }

  ImGui::End();
}
} // namespace imgui
} // namespace engine
