#include "imgui_utils.hpp"
#include "asset_manager.hpp"
#include "camera_component.hpp"
#include "gltf_model_component.hpp"
#include "light_component.hpp"
#include "skybox_component.hpp"
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

void assetsWindow(AssetManager &assetManager) {
  ImGui::Begin("Assets");

  for (auto &[name, info] : assetManager.getAssetTable()) {
    std::string s = std::to_string(info.assetIndex) + ": " + name;
    ImGui::Selectable(s.c_str(), false);
  }

  ImGui::End();
}

void entitiesWindow(ecs::World &world) {
  ImGui::Begin("Entities");
  world.each([&](ecs::Entity entity) {
    ImGui::PushID(entity);
    ImGui::Text("Entity #%ld", entity);
    ImGui::Indent();

    auto transform = world.getComponent<TransformComponent>(entity);
    if (transform != nullptr) {
      if (ImGui::CollapsingHeader("Transform component")) {
        ImGui::Indent();
        float pos[] = {
            transform->position.x,
            transform->position.y,
            transform->position.z,
        };
        ImGui::DragFloat3("Position", pos, 0.1f);
        transform->position = glm::vec3(pos[0], pos[1], pos[2]);

        glm::vec3 euler = glm::degrees(glm::eulerAngles(transform->rotation));
        float rot[] = {euler.x, euler.y, euler.z};
        ImGui::DragFloat3("Rotation", rot, 1.0f);
        euler = glm::radians(glm::vec3(rot[0], rot[1], rot[2]));
        transform->rotation = glm::quat(euler);

        float scale[] = {
            transform->scale.x,
            transform->scale.y,
            transform->scale.z,
        };
        ImGui::DragFloat3("Scale", scale, 0.1f);
        transform->scale = glm::vec3(scale[0], scale[1], scale[2]);

        ImGui::Unindent();
      }
    }

    auto gltfModel = world.getComponent<GltfModelComponent>(entity);
    if (gltfModel != nullptr) {
      if (ImGui::CollapsingHeader("Model component")) {
        ImGui::Indent();
        ImGui::Text("Test");
        ImGui::Unindent();
      }
    }

    auto camera = world.getComponent<CameraComponent>(entity);
    if (camera != nullptr) {
      if (ImGui::CollapsingHeader("Camera component")) {
        ImGui::Indent();
        ImGui::DragFloat("FOV", &camera->m_fov);
        ImGui::Unindent();
      }
    }

    auto skybox = world.getComponent<SkyboxComponent>(entity);
    if (skybox != nullptr) {
      if (ImGui::CollapsingHeader("Skybox component")) {
        ImGui::Indent();
        ImGui::DragFloat("Exposure", &skybox->m_environmentUBO.exposure);
        ImGui::Unindent();
      }
    }

    auto light = world.getComponent<LightComponent>(entity);
    if (light != nullptr) {
      if (ImGui::CollapsingHeader("Light component")) {
        ImGui::Indent();
        float color[3] = {
            light->color.x,
            light->color.y,
            light->color.z,
        };
        ImGui::ColorEdit3("Color", color);
        light->color = {color[0], color[1], color[2]};
        ImGui::Unindent();
      }
    }

    ImGui::Unindent();
    ImGui::PopID();
  });
  ImGui::End();
}

} // namespace imgui
} // namespace engine
