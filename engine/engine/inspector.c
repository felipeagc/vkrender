#include "inspector.h"
#include "asset_manager.h"
#include "assets/environment_asset.h"
#include "assets/gltf_model_asset.h"
#include "assets/mesh_asset.h"
#include "assets/pbr_material_asset.h"
#include "components/gltf_model_component.h"
#include "components/mesh_component.h"
#include "components/transform_component.h"
#include "world.h"
#include <renderer/imgui.h>
#include <renderer/window.h>
#include <stdio.h>

#define INDENT_LEVEL 8.0f

void eg_draw_inspector(
    re_window_t *window, eg_world_t *world, eg_asset_manager_t *asset_manager) {
  static char header_title[256] = "";

  if (igBegin("Inspector", NULL, 0)) {
    if (igBeginTabBar("Inspector", 0)) {

      if (igBeginTabItem("World", NULL, 0)) {
        if (igCollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
          float deg = to_degrees(world->camera.fov);
          igDragFloat("FOV", &deg, 0.1f, 0.0f, 0.0f, "%.3f", 1.0f);
          world->camera.fov = to_radians(deg);
        }

        if (igCollapsingHeader("Environment", ImGuiTreeNodeFlags_DefaultOpen)) {
          igDragFloat3(
              "Sun direction",
              &world->environment.uniform.sun_direction.x,
              0.01f,
              0.0f,
              0.0f,
              "%.3f",
              1.0f);

          igColorEdit3("Sun color", &world->environment.uniform.sun_color.x, 0);

          igDragFloat(
              "Sun intensity",
              &world->environment.uniform.sun_intensity,
              0.01f,
              0.0f,
              0.0f,
              "%.3f",
              1.0f);

          igDragFloat(
              "Exposure",
              &world->environment.uniform.exposure,
              0.01f,
              0.0f,
              0.0f,
              "%.3f",
              1.0f);
        }

        igEndTabItem();
      }

      if (igBeginTabItem("Statistics", NULL, 0)) {
        igText("Delta time: %.4fms", window->delta_time);
        igText("FPS: %.2f", 1.0f / window->delta_time);
        igEndTabItem();
      }

      if (igBeginTabItem("Entities", NULL, 0)) {
        for (eg_entity_t entity = 0; entity < EG_MAX_ENTITIES; entity++) {
          if (!eg_world_has_any_comp(world, entity)) {
            continue;
          }

          snprintf(header_title, sizeof(header_title), "Entity: %d", entity);
          if (igCollapsingHeader(header_title, 0)) {
            igPushIDInt(entity);
            igIndent(INDENT_LEVEL);

            if (eg_world_has_comp(world, entity, EG_TRANSFORM_COMPONENT_TYPE)) {
              eg_transform_component_t *transform =
                  EG_GET_COMP(world, entity, eg_transform_component_t);

              if (igCollapsingHeader(
                      "Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                igDragFloat3(
                    "Position",
                    &transform->position.x,
                    0.1f,
                    0.0f,
                    0.0f,
                    "%.3f",
                    1.0f);
                igDragFloat3(
                    "Scale",
                    &transform->scale.x,
                    0.1f,
                    0.0f,
                    0.0f,
                    "%.3f",
                    1.0f);
                igDragFloat3(
                    "Axis",
                    &transform->axis.x,
                    0.01f,
                    0.0f,
                    0.0f,
                    "%.3f",
                    1.0f);
                igDragFloat(
                    "Angle",
                    &transform->angle,
                    0.01f,
                    0.0f,
                    0.0f,
                    "%.3f rad",
                    1.0f);

                transform->axis = vec3_normalize(transform->axis);
              }
            }

            if (eg_world_has_comp(
                    world, entity, EG_GLTF_MODEL_COMPONENT_TYPE)) {
              eg_gltf_model_component_t *gltf_model =
                  EG_GET_COMP(world, entity, eg_gltf_model_component_t);

              if (igCollapsingHeader(
                      "GLTF Model", ImGuiTreeNodeFlags_DefaultOpen)) {
                igText("Asset: %s", gltf_model->asset->asset.name);
              }
            }

            igUnindent(INDENT_LEVEL);
            igPopID();
          }

          igSeparator();
        }

        igEndTabItem();
      }

      if (igBeginTabItem("Assets", NULL, 0)) {
        for (uint32_t i = 0; i < asset_manager->asset_count; i++) {
          eg_asset_t *asset = asset_manager->assets[i];
          igPushIDInt(i);

          if (asset->type == EG_ENVIRONMENT_ASSET_TYPE) {
            snprintf(
                header_title,
                sizeof(header_title),
                "Environment: %s",
                asset->name);
            if (igCollapsingHeader(header_title, 0)) {
            }
          }

          if (asset->type == EG_PBR_MATERIAL_ASSET_TYPE) {
            eg_pbr_material_asset_t *material =
                (eg_pbr_material_asset_t *)asset;

            snprintf(
                header_title,
                sizeof(header_title),
                "Material: %s",
                asset->name);
            if (igCollapsingHeader(header_title, 0)) {
              igColorEdit4("Color", &material->uniform.base_color_factor.x, 0);
              igDragFloat(
                  "Metallic",
                  &material->uniform.metallic,
                  0.01f,
                  0.0f,
                  1.0f,
                  "%.3f",
                  1.0f);
              igDragFloat(
                  "Roughness",
                  &material->uniform.roughness,
                  0.01f,
                  0.0f,
                  1.0f,
                  "%.3f",
                  1.0f);
              igColorEdit4(
                  "Emissive factor", &material->uniform.emissive_factor.x, 0);
            }
          }

          if (asset->type == EG_MESH_ASSET_TYPE) {
          }

          if (asset->type == EG_GLTF_MODEL_ASSET_TYPE) {
            snprintf(
                header_title,
                sizeof(header_title),
                "GLTF model: %s",
                asset->name);
            if (igCollapsingHeader(header_title, 0)) {
              eg_gltf_model_asset_t *gltf_asset =
                  (eg_gltf_model_asset_t *)asset;

              for (uint32_t j = 0; j < gltf_asset->material_count; j++) {
                igPushIDInt(j);
                eg_gltf_model_asset_material_t *material =
                    &gltf_asset->materials[j];

                if (igCollapsingHeader(
                        "Material", ImGuiTreeNodeFlags_DefaultOpen)) {
                  igColorEdit4(
                      "Color", &material->uniform.base_color_factor.x, 0);
                  igDragFloat(
                      "Metallic",
                      &material->uniform.metallic,
                      0.01f,
                      0.0f,
                      1.0f,
                      "%.3f",
                      1.0f);
                  igDragFloat(
                      "Roughness",
                      &material->uniform.roughness,
                      0.01f,
                      0.0f,
                      1.0f,
                      "%.3f",
                      1.0f);
                  igColorEdit4(
                      "Emissive factor",
                      &material->uniform.emissive_factor.x,
                      0);
                }

                igPopID();
              }
            }
          }

          igPopID();
          igSeparator();
        }

        igEndTabItem();
      }

      igEndTabBar();
    }

    igEnd();
  }
}
