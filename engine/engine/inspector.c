#include "inspector.h"
#include "asset_manager.h"
#include "assets/environment_asset.h"
#include "assets/mesh_asset.h"
#include "assets/pbr_material_asset.h"
#include "components/mesh_component.h"
#include "components/transform_component.h"
#include "world.h"
#include <renderer/imgui.h>
#include <stdio.h>

void eg_draw_inspector(eg_world_t *world, eg_asset_manager_t *asset_manager) {
  if (igBegin("Inspector", NULL, 0)) {
    if (igBeginTabBar("Inspector", 0)) {

      if (igBeginTabItem("Camera", NULL, 0)) {
        float deg = to_degrees(world->camera.fov);
        igDragFloat("FOV", &deg, 0.1f, 0.0f, 0.0f, "%.3f", 1.0f);
        world->camera.fov = to_radians(deg);
        igEndTabItem();
      }

      if (igBeginTabItem("Environment", NULL, 0)) {
        igDragFloat3(
            "Sun direction",
            &world->environment.uniform.sun_direction.x,
            0.01f,
            0.0f,
            0.0f,
            "%.3f",
            1.0f);

        igColorEdit3(
            "Sun color", &world->environment.uniform.sun_color.x, 0);

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

        igEndTabItem();
      }

      if (igBeginTabItem("Entities", NULL, 0)) {
        for (eg_entity_t entity = 0; entity < EG_MAX_ENTITIES; entity++) {
          if (!eg_world_has_any_comp(world, entity)) {
            continue;
          }

          igPushIDInt(entity);

          static char header_title[256] = "";
          snprintf(header_title, 256, "Entity: %d", entity);

          if (igCollapsingHeader(header_title, 0)) {
            igBeginTabBar("Components", 0);

            if (eg_world_has_comp(world, entity, EG_TRANSFORM_COMPONENT_TYPE)) {
              eg_transform_component_t *transform =
                  EG_GET_COMP(world, entity, eg_transform_component_t);

              if (igBeginTabItem("Transform", NULL, 0)) {
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
                    "Angle", &transform->angle, 0.1f, 0.0f, 0.0f, "%.3f", 1.0f);

                transform->axis = vec3_normalize(transform->axis);

                igEndTabItem();
              }
            }

            igEndTabBar();
          }

          igPopID();
          igSeparator();
        }

        igEndTabItem();
      }

      if (igBeginTabItem("Assets", NULL, 0)) {
        for (uint32_t i = 0; i < asset_manager->asset_count; i++) {
          eg_asset_t *asset = asset_manager->assets[i];
          igPushIDInt(i);

          if (asset->type == EG_ENVIRONMENT_ASSET_TYPE) {
            igText("Environment: %s", asset->name);
          }

          if (asset->type == EG_PBR_MATERIAL_ASSET_TYPE) {
            eg_pbr_material_asset_t *material =
                (eg_pbr_material_asset_t *)asset;

            igText("Material: %s", material->asset.name);
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

          if (asset->type == EG_MESH_ASSET_TYPE) {
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
