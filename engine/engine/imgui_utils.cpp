#include "imgui_utils.hpp"
#include "asset_manager.hpp"
#include "camera_component.hpp"
#include "light_component.hpp"
#include "light_manager.hpp"
#include "transform_component.hpp"
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

void cameraWindow(CameraComponent *camera, TransformComponent *transform) {
  ImGui::Begin("Camera");

  ImGui::DragFloat("FOV", &camera->m_fov);

  float camPos[] = {
      transform->position.x, transform->position.y, transform->position.z};
  ImGui::DragFloat3("Camera position", camPos, 0.1f, -10.0f, 10.0f);
  transform->position.x = camPos[0];
  transform->position.y = camPos[1];
  transform->position.z = camPos[2];

  glm::vec3 euler = glm::degrees(glm::eulerAngles(transform->rotation));
  float camRot[] = {euler.x, euler.y, euler.z};
  ImGui::DragFloat3("Camera rotation", camRot, 1.0f, -180.0f, 180.0f);

  ImGui::End();
}

void assetsWindow(AssetManager &assetManager) {
  ImGui::Begin("Assets");

  for (auto &[name, info] : assetManager.getAssetTable()) {
    std::string s = std::to_string(info.assetIndex) + ": " + name;
    ImGui::Selectable(s.c_str(), false);
  }

  ImGui::End();
}

void lightSection(
    ecs::Entity entity, TransformComponent &transform, LightComponent &light) {
  ImGui::PushID(entity);
  ImGui::Text("Light entity %ld", entity);

  float pos[3] = {
      transform.position.x,
      transform.position.y,
      transform.position.z,
  };
  ImGui::DragFloat3("Position", pos, 0.1f, -10.0f, 10.0f);
  transform.position = {pos[0], pos[1], pos[2]};

  float color[3] = {
      light.color.x,
      light.color.y,
      light.color.z,
  };
  ImGui::ColorEdit3("Color", color);
  light.color = {color[0], color[1], color[2]};

  ImGui::Separator();
  ImGui::PopID();
}

} // namespace imgui
} // namespace engine
