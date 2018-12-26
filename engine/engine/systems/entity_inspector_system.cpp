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
        ImGui::DragFloat3("Position", &transform->position.x, 0.1f);

        float quat[] = {
            transform->rotation.w,
            transform->rotation.x,
            transform->rotation.y,
            transform->rotation.z,
        };
        ImGui::DragFloat4("Quaternion", quat);

        if (ImGui::Button("Reset rotation")) {
          transform->rotation = glm::quat(1.0, 0.0, 0.0, 0.0f);
        }

        ImGui::SameLine();

        static float angle;
        static glm::vec3 axis;
        static glm::quat prevQuat;

        if (ImGui::Button("Apply rotation")) {
          ImGui::OpenPopup("Apply rotation");
          prevQuat = transform->rotation;
          angle = 0.0f;
          axis = glm::vec3(0.0f, 0.0f, 1.0f);
        }

        if (ImGui::BeginPopup("Apply rotation")) {

          ImGui::DragFloat("Angle", &angle, 1.0f);
          ImGui::SameLine();
          ImGui::DragFloat3("Rotation axis", &axis.x, 0.1f, -1.0f, 1.0f);

          transform->rotation =
            glm::rotate(prevQuat, glm::radians(angle), glm::normalize(axis));

          ImGui::EndPopup();
        }

        ImGui::DragFloat3("Scale", &transform->scale.x, 0.1f);

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
