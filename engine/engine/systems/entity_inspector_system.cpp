#include "entity_inspector_system.hpp"

#include "../assets/shape_asset.hpp"
#include "../components/camera_component.hpp"
#include "../components/environment_component.hpp"
#include "../components/gltf_model_component.hpp"
#include "../components/light_component.hpp"
#include "../components/name_component.hpp"
#include "../components/transform_component.hpp"
#include <ecs/world.hpp>
#include <imgui/imgui.h>
#include <renderer/window.hpp>

using namespace engine;

EntityInspectorSystem::EntityInspectorSystem(AssetManager &assetManager) {
  m_boxAsset = assetManager
                   .loadAsset<engine::ShapeAsset>(
                       engine::BoxShape(glm::vec3(-1.0), glm::vec3(1.0)))
                   .index;
}

EntityInspectorSystem::~EntityInspectorSystem() {}

static inline void entityInspector(ecs::World &world, ecs::Entity entity) {
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
        ImGui::DragFloat3("Rotation axis", &axis.x, 0.01f, -1.0f, 1.0f);

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
      ImGui::DragFloat(
          "Exposure", &environment->m_ubo.exposure, 0.5f, 0.0f, 1000.0f);
      ImGui::DragFloat3(
          "Sun direction",
          &environment->m_ubo.sunDirection[0],
          0.01f,
          -1.0f,
          1.0f);
      ImGui::ColorEdit3("Sun color", &environment->m_ubo.sunColor[0]);
      ImGui::DragFloat(
          "Sun intensity",
          &environment->m_ubo.sunIntensity,
          0.01f,
          0.0f,
          100.0f);
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
}

void EntityInspectorSystem::imgui(ecs::World &world) {
  ImGui::Begin("Entities");
  char buf[256] = "";
  world.each([&](ecs::Entity entity) {
    auto name = world.getComponent<NameComponent>(entity);
    sprintf(buf, "Entity #%lu", entity);
    if (name->name.length() > 0) {
      strcat(buf, ": ");
      strcat(buf, name->name.c_str());
    }

    bool selected = m_selectedEntity == entity;
    if (ImGui::Selectable(buf, entity, selected)) {
      m_selectedEntity = entity;
    }
  });
  ImGui::End();

  if (m_selectedEntity != ecs::MAX_ENTITY) {
    ImGui::Begin("Entity inspector");
    entityInspector(world, m_selectedEntity);
    ImGui::End();
  }
}

void EntityInspectorSystem::drawBox(
    renderer::Window &window,
    AssetManager &assetManager,
    ecs::World &world,
    renderer::GraphicsPipeline &boxPipeline) {
  if (m_selectedEntity == ecs::MAX_ENTITY) {
    return;
  }

  auto model = world.getComponent<engine::GltfModelComponent>(m_selectedEntity);

  if (model == nullptr) {
    return;
  }

  ecs::Entity camera = world.first<engine::CameraComponent>();
  world.getComponent<engine::CameraComponent>(camera)->bind(
      window, boxPipeline);

  auto &box = assetManager.getAsset<ShapeAsset>(m_boxAsset);

  vkCmdBindPipeline(
      window.getCurrentCommandBuffer(),
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      boxPipeline.pipeline);

  auto &modelAsset = assetManager.getAsset<GltfModelAsset>(model->m_modelIndex);

  struct {
    alignas(16) glm::vec3 scale;
    alignas(16) glm::vec3 offset;
  } pushConstant;
  pushConstant.scale = (modelAsset.dimensions.size) / 2.0f;
  pushConstant.offset = modelAsset.dimensions.center;

  vkCmdPushConstants(
      window.getCurrentCommandBuffer(),
      boxPipeline.layout,
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      0,
      sizeof(pushConstant),
      &pushConstant);

  VkDeviceSize offset = 0;
  VkBuffer vertexBuffer = box.m_vertexBuffer.getHandle();
  vkCmdBindVertexBuffers(
      window.getCurrentCommandBuffer(), 0, 1, &vertexBuffer, &offset);

  vkCmdBindIndexBuffer(
      window.getCurrentCommandBuffer(),
      box.m_indexBuffer.getHandle(),
      0,
      VK_INDEX_TYPE_UINT32);

  vkCmdBindDescriptorSets(
      window.getCurrentCommandBuffer(),
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      boxPipeline.layout,
      1, // firstSet
      1,
      model->m_descriptorSets[window.getCurrentFrameIndex()],
      0,
      nullptr);

  vkCmdDrawIndexed(
      window.getCurrentCommandBuffer(),
      box.m_shape.m_indices.size(),
      1,
      0,
      0,
      0);
}

static inline glm::vec3 rayCast(
    const float mx,
    const float my,
    const float width,
    const float height,
    const glm::mat4 &view,
    const glm::mat4 &proj) {
  float x = (2.0f * mx) / width - 1.0f;
  float y = (2.0f * my) / height - 1.0f;

  glm::vec4 clip(x, y, -1.0f, 0.0f);

  glm::vec4 eye = glm::inverse(proj) * clip;

  // @hack: I just tried this 2/3 number and it worked. No idea why.
  eye.z = -(2.0f / 3.0f);
  eye.w = 0.0f;

  glm::vec4 ray = glm::inverse(view) * eye;
  return glm::normalize(glm::vec3(ray.x, ray.y, ray.z));
}

static inline bool intersect(
    glm::vec3 rayOrigin,
    glm::vec3 rayDir,
    glm::vec3 aabbMin,
    glm::vec3 aabbMax,
    glm::vec3 objPos,
    glm::mat4 modelMatrix,
    float *dist) {
  float tMin = 0.0f;
  float tMax = FLT_MAX;

  glm::vec3 delta = objPos - rayOrigin;

  {
    glm::vec3 xaxis{modelMatrix[0].x, modelMatrix[0].y, modelMatrix[0].z};
    xaxis = glm::normalize(xaxis);
    float e = glm::dot(xaxis, delta);
    float f = glm::dot(rayDir, xaxis);

    if (fabs(f) > 0.0001f) {
      float t1 = (e + aabbMin.x) / f;
      float t2 = (e + aabbMax.x) / f;

      if (t1 > t2) {
        // Swap t1 and t2
        float w = t1;
        t1 = t2;
        t2 = w;
      }

      if (t2 < tMax)
        tMax = t2;
      if (t1 > tMin)
        tMin = t1;

      if (tMax < tMin) {
        return false;
      }
    } else {
      if (-e + aabbMin.x > 0.0f || -e + aabbMax.x < 0.0f) {
        return false;
      }
    }
  }

  {
    glm::vec3 yaxis{modelMatrix[1].x, modelMatrix[1].y, modelMatrix[1].z};
    yaxis = glm::normalize(yaxis);
    float e = glm::dot(yaxis, delta);
    float f = glm::dot(rayDir, yaxis);

    if (fabs(f) > 0.0001f) {
      float t1 = (e + aabbMin.y) / f;
      float t2 = (e + aabbMax.y) / f;

      if (t1 > t2) {
        // Swap t1 and t2
        float w = t1;
        t1 = t2;
        t2 = w;
      }

      if (t2 < tMax)
        tMax = t2;
      if (t1 > tMin)
        tMin = t1;

      if (tMax < tMin) {
        return false;
      }
    } else {
      if (-e + aabbMin.y > 0.0f || -e + aabbMax.y < 0.0f) {
        return false;
      }
    }
  }

  {
    glm::vec3 zaxis{modelMatrix[2].x, modelMatrix[2].y, modelMatrix[2].z};
    zaxis = glm::normalize(zaxis);
    float e = glm::dot(zaxis, delta);
    float f = glm::dot(rayDir, zaxis);

    if (fabs(f) > 0.0001f) {
      float t1 = (e + aabbMin.z) / f;
      float t2 = (e + aabbMax.z) / f;

      if (t1 > t2) {
        // Swap t1 and t2
        float w = t1;
        t1 = t2;
        t2 = w;
      }

      if (t2 < tMax)
        tMax = t2;
      if (t1 > tMin)
        tMin = t1;

      if (tMax < tMin) {
        return false;
      }
    } else {
      if (-e + aabbMin.z > 0.0f || -e + aabbMax.z < 0.0f) {
        return false;
      }
    }
  }

  *dist = tMin;
  return true;
}

void EntityInspectorSystem::processEvent(
    const renderer::Window &window,
    AssetManager &assetManager,
    ecs::World &world,
    const SDL_Event &event) {
  if (event.type == SDL_MOUSEBUTTONDOWN) {
    if (event.button.button == SDL_BUTTON_LEFT) {
      if (ImGui::IsAnyWindowHovered()) {
        return;
      }

      float mouseX = (float)event.button.x;
      float mouseY = (float)event.button.y;

      auto camera =
          world.first<engine::CameraComponent, engine::TransformComponent>();
      auto cameraComp = world.getComponent<engine::CameraComponent>(camera);
      auto cameraTransform =
          world.getComponent<engine::TransformComponent>(camera);

      glm::vec3 rayOrigin = cameraTransform->position;

      glm::vec3 rayDir = rayCast(
          mouseX,
          mouseY,
          (float)window.getWidth(),
          (float)window.getHeight(),
          cameraComp->m_cameraUniform.view,
          cameraComp->m_cameraUniform.proj);

      m_selectedEntity = ecs::MAX_ENTITY;

      ftl::small_vector<std::pair<float, ecs::Entity>, 16> entities;

      world.each<TransformComponent, GltfModelComponent>(
          [&](ecs::Entity entity,
              TransformComponent &transform,
              GltfModelComponent &gltfModel) {
            auto &modelAsset =
                assetManager.getAsset<GltfModelAsset>(gltfModel.m_modelIndex);

            glm::vec3 scale = (modelAsset.dimensions.size) / 2.0f;
            glm::vec3 offset = modelAsset.dimensions.center;

            glm::vec3 aabbMin = glm::vec3{-1.0} * scale + offset;
            glm::vec3 aabbMax = glm::vec3{1.0} * scale + offset;

            aabbMin *= transform.scale;
            aabbMax *= transform.scale;

            float dist = FLT_MAX;
            if (intersect(
                    rayOrigin,
                    rayDir,
                    aabbMin,
                    aabbMax,
                    transform.position,
                    transform.getMatrix(),
                    &dist)) {
              entities.push_back({dist, entity});
            }
          });

      if (entities.size() > 0) {
        std::sort(entities.begin(), entities.end());

        m_selectedEntity = entities[0].second;
      }
    }
  }
}
