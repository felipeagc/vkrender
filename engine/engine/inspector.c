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

static void inspector_camera(eg_camera_t *camera) {
  float deg = to_degrees(camera->fov);
  igDragFloat("FOV", &deg, 0.1f, 0.0f, 0.0f, "%.3f", 1.0f);
  camera->fov = to_radians(deg);
}

static void inspector_environment(eg_environment_t *environment) {
  igDragFloat3(
      "Sun direction",
      &environment->uniform.sun_direction.x,
      0.01f,
      0.0f,
      0.0f,
      "%.3f",
      1.0f);

  igColorEdit3("Sun color", &environment->uniform.sun_color.x, 0);

  igDragFloat(
      "Sun intensity",
      &environment->uniform.sun_intensity,
      0.01f,
      0.0f,
      0.0f,
      "%.3f",
      1.0f);

  igDragFloat(
      "Exposure",
      &environment->uniform.exposure,
      0.01f,
      0.0f,
      0.0f,
      "%.3f",
      1.0f);
}

static void inspector_statistics(re_window_t *window) {
  igText("Delta time: %.4fms", window->delta_time);
  igText("FPS: %.2f", 1.0f / window->delta_time);
}

static void inspector_environment_asset(eg_asset_t *asset) {}

static void inspector_pbr_material_asset(eg_asset_t *asset) {
  eg_pbr_material_asset_t *material = (eg_pbr_material_asset_t *)asset;

  igColorEdit4("Color", &material->uniform.base_color_factor.x, 0);
  igDragFloat(
      "Metallic", &material->uniform.metallic, 0.01f, 0.0f, 1.0f, "%.3f", 1.0f);
  igDragFloat(
      "Roughness",
      &material->uniform.roughness,
      0.01f,
      0.0f,
      1.0f,
      "%.3f",
      1.0f);
  igColorEdit4("Emissive factor", &material->uniform.emissive_factor.x, 0);
}

static void inspector_gltf_model_asset(eg_asset_t *asset) {
  eg_gltf_model_asset_t *gltf_asset = (eg_gltf_model_asset_t *)asset;

  igText("Vertex count: %u", gltf_asset->vertex_count);
  igText("Index count: %u", gltf_asset->index_count);
  igText("Image count: %u", gltf_asset->image_count);
  igText("Mesh count: %u", gltf_asset->mesh_count);

  for (uint32_t j = 0; j < gltf_asset->material_count; j++) {
    igPushIDInt(j);
    eg_gltf_model_asset_material_t *material = &gltf_asset->materials[j];

    if (igCollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
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
      igColorEdit4("Emissive factor", &material->uniform.emissive_factor.x, 0);
    }
    igPopID();
  }
}

static void
inspector_transform_component(eg_world_t *world, eg_entity_t entity) {
  eg_transform_component_t *transform =
      EG_GET_COMP(world, entity, eg_transform_component_t);

  igDragFloat3(
      "Position", &transform->position.x, 0.1f, 0.0f, 0.0f, "%.3f", 1.0f);
  igDragFloat3("Scale", &transform->scale.x, 0.1f, 0.0f, 0.0f, "%.3f", 1.0f);
  igDragFloat3("Axis", &transform->axis.x, 0.01f, 0.0f, 1.0f, "%.3f", 1.0f);
  igDragFloat("Angle", &transform->angle, 0.01f, 0.0f, 0.0f, "%.3f rad", 1.0f);
}

static void
inspector_gltf_model_component(eg_world_t *world, eg_entity_t entity) {
  eg_gltf_model_component_t *gltf_model =
      EG_GET_COMP(world, entity, eg_gltf_model_component_t);

  igText("Asset: %s", gltf_model->asset->asset.name);
  igSameLine(0.0f, -1.0f);
  if (igSmallButton("Inspect")) {
    igOpenPopup("gltfmodelpopup");
  }

  if (igBeginPopup("gltfmodelpopup", 0)) {
    inspector_gltf_model_asset((eg_asset_t *)gltf_model->asset);
    igEndPopup();
  }
}

static void inspector_mesh_component(eg_world_t *world, eg_entity_t entity) {}

void eg_inspector_init(eg_inspector_t *inspector) {
  inspector->selected_entity = UINT32_MAX;
}

void eg_draw_inspector(
    eg_inspector_t *inspector,
    re_window_t *window,
    eg_world_t *world,
    eg_asset_manager_t *asset_manager) {
  static char str[256] = "";

  if (inspector->selected_entity != UINT32_MAX) {
    if (igBegin("Selected entity", NULL, 0)) {
      eg_entity_t entity = inspector->selected_entity;

      igText("Entity #%u", inspector->selected_entity);

      if (eg_world_has_comp(world, entity, EG_TRANSFORM_COMPONENT_TYPE) &&
          igCollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        inspector_transform_component(world, entity);
      }

      if (eg_world_has_comp(world, entity, EG_MESH_COMPONENT_TYPE) &&
          igCollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen)) {
        inspector_mesh_component(world, entity);
      }

      if (eg_world_has_comp(world, entity, EG_GLTF_MODEL_COMPONENT_TYPE) &&
          igCollapsingHeader("GLTF Model", ImGuiTreeNodeFlags_DefaultOpen)) {
        inspector_gltf_model_component(world, entity);
      }
    }
    igEnd();
  }

  if (igBegin("Inspector", NULL, 0)) {
    if (igBeginTabBar("Inspector", 0)) {
      if (igBeginTabItem("World", NULL, 0)) {
        if (igCollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
          inspector_camera(&world->camera);
        }

        if (igCollapsingHeader("Environment", ImGuiTreeNodeFlags_DefaultOpen)) {
          inspector_environment(&world->environment);
        }

        igEndTabItem();
      }

      if (igBeginTabItem("Statistics", NULL, 0)) {
        inspector_statistics(window);
        igEndTabItem();
      }

      if (igBeginTabItem("Entities", NULL, 0)) {
        for (eg_entity_t entity = 0; entity < EG_MAX_ENTITIES; entity++) {
          if (!eg_world_has_any_comp(world, entity)) {
            continue;
          }

          snprintf(str, sizeof(str), "Entity #%d", entity);
          if (igSelectable(str, false, 0, (ImVec2){0.0f, 0.0f})) {
            inspector->selected_entity = entity;
          }
        }

        igEndTabItem();
      }

      if (igBeginTabItem("Assets", NULL, 0)) {
        for (uint32_t i = 0; i < EG_MAX_ASSETS; i++) {
          eg_asset_t *asset = eg_asset_manager_get_by_index(asset_manager, i);
          if (asset == NULL) {
            continue;
          }

          igPushIDInt(i);

#define ASSET_HEADER(format, ...)                                              \
  snprintf(str, sizeof(str), format, __VA_ARGS__);                             \
  if (igCollapsingHeader(str, 0))

          switch (asset->type) {
          case EG_ENVIRONMENT_ASSET_TYPE: {
            ASSET_HEADER("Environment: %s", asset->name) {
              inspector_environment_asset(asset);
            }
            break;
          }
          case EG_PBR_MATERIAL_ASSET_TYPE: {
            ASSET_HEADER("Material: %s", asset->name) {
              inspector_pbr_material_asset(asset);
            }
            break;
          }
          case EG_MESH_ASSET_TYPE: {
            ASSET_HEADER("Mesh: %s", asset->name) {}
            break;
          }
          case EG_GLTF_MODEL_ASSET_TYPE: {
            ASSET_HEADER("GLTF model: %s", asset->name) {
              inspector_gltf_model_asset(asset);
            }
            break;
          }
          default:
            break;
          }

          igPopID();
          igSeparator();
        }

        igEndTabItem();
      }

      igEndTabBar();
    }
  }

  igEnd();
}
