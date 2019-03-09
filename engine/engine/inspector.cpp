#include "inspector.hpp"
#include "asset_manager.hpp"
#include "assets/environment_asset.hpp"
#include "assets/mesh_asset.hpp"
#include "assets/pbr_material_asset.hpp"
#include "components/mesh_component.hpp"
#include "components/transform_component.hpp"
#include "world.hpp"
#include <imgui.h>
#include <stdio.h>

void eg_draw_inspector(eg_world_t *world, eg_asset_manager_t *asset_manager) {
  if (ImGui::Begin("Inspector")) {
    if (ImGui::BeginTabBar("Inspector")) {

      if (ImGui::BeginTabItem("Camera")) {
        float deg = to_degrees(world->camera.fov);
        ImGui::DragFloat("FOV", &deg, 0.1f);
        world->camera.fov = to_radians(deg);
        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Environment")) {
        ImGui::DragFloat3(
            "Sun direction",
            &world->environment.uniform.sun_direction.x,
            0.01f);

        ImGui::ColorEdit3("Sun color", &world->environment.uniform.sun_color.x);

        ImGui::DragFloat(
            "Sun intensity", &world->environment.uniform.sun_intensity, 0.01f);

        ImGui::DragFloat(
            "Exposure", &world->environment.uniform.exposure, 0.01f);

        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Entities")) {
        for (eg_entity_t entity = 0; entity < EG_MAX_ENTITIES; entity++) {
          if (!eg_world_has_any_comp(world, entity)) {
            continue;
          }

          ImGui::PushID(entity);

          static char header_title[256] = "";
          snprintf(header_title, 256, "Entity: %d", entity);

          if (ImGui::CollapsingHeader(header_title)) {
            ImGui::BeginTabBar("Components");

            if (eg_world_has_comp(world, entity, EG_TRANSFORM_COMPONENT_TYPE)) {
              eg_transform_component_t *transform =
                  EG_GET_COMP(world, entity, eg_transform_component_t);

              if (ImGui::BeginTabItem("Transform")) {
                ImGui::DragFloat3("Position", &transform->position.x, 0.1f);
                ImGui::DragFloat3("Scale", &transform->scale.x, 0.1f);
                ImGui::DragFloat3("Axis", &transform->axis.x, 0.01f);
                ImGui::DragFloat("Angle", &transform->angle, 0.1f);

                transform->axis = vec3_normalize(transform->axis);

                ImGui::EndTabItem();
              }
            }

            ImGui::EndTabBar();
          }

          ImGui::PopID();
          ImGui::Separator();
        }

        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Assets")) {
        for (uint32_t i = 0; i < asset_manager->asset_count; i++) {
          eg_asset_t *asset = asset_manager->assets[i];
          ImGui::PushID(i);

          if (asset->type == EG_ENVIRONMENT_ASSET_TYPE) {
            ImGui::Text("Environment: %s", asset->name);
          }

          if (asset->type == EG_PBR_MATERIAL_ASSET_TYPE) {
            eg_pbr_material_asset_t *material =
                (eg_pbr_material_asset_t *)asset;

            ImGui::Text("Material: %s", material->asset.name);
            ImGui::ColorEdit4("Color", &material->uniform.base_color_factor.x);
            ImGui::DragFloat(
                "Metallic", &material->uniform.metallic, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat(
                "Roughness", &material->uniform.roughness, 0.01f, 0.0f, 1.0f);
            ImGui::ColorEdit4(
                "Emissive factor", &material->uniform.emissive_factor.x);
          }

          if (asset->type == EG_MESH_ASSET_TYPE) {
          }

          ImGui::PopID();
          ImGui::Separator();
        }

        ImGui::EndTabItem();
      }

      ImGui::EndTabBar();
    }

    ImGui::End();
  }
}
