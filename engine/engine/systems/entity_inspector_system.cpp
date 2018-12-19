#include "entity_inspector_system.hpp"

#include "../components/camera_component.hpp"
#include "../components/environment_component.hpp"
#include "../components/gltf_model_component.hpp"
#include "../components/light_component.hpp"
#include "../components/transform_component.hpp"
#include <imgui/imgui.h>

using namespace engine;

EntityInspectorSystem::EntityInspectorSystem() {}

EntityInspectorSystem::~EntityInspectorSystem() {}

void EntityInspectorSystem::process(ecs::World &world) {
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
        ImGui::DragFloat("FOV", &camera->m_fov, 1.0f, 0.0f, 180.0f);
        ImGui::Unindent();
      }
    }

    auto environment = world.getComponent<EnvironmentComponent>(entity);
    if (environment != nullptr) {
      if (ImGui::CollapsingHeader("Environment component")) {
        ImGui::Indent();
        float exposure = environment->getExposure();
        ImGui::DragFloat("Exposure", &exposure, 0.5f, 0.0f, 1000.0f);
        environment->setExposure(exposure);
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

        ImGui::DragFloat("Intensity", &light->intensity, 0.1f, 0.0f, 1000.0f);
        ImGui::Unindent();
      }
    }

    ImGui::Unindent();
    ImGui::PopID();
  });
  ImGui::End();
}
