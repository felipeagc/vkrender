#include "pbr_material_asset.h"

#include "../asset_manager.h"
#include "../engine.h"
#include "../imgui.h"
#include "../pipelines.h"
#include <fstd_util.h>
#include <renderer/context.h>
#include <renderer/image.h>
#include <renderer/pipeline.h>
#include <renderer/util.h>
#include <renderer/window.h>
#include <string.h>

eg_pbr_material_asset_t *eg_pbr_material_asset_create(
    eg_asset_manager_t *asset_manager,
    eg_pbr_material_asset_options_t *options) {
  eg_pbr_material_asset_t *material = eg_asset_manager_alloc(
      asset_manager, EG_ASSET_TYPE(eg_pbr_material_asset_t));

  material->uniform.base_color_factor  = (vec4_t){1.0, 1.0, 1.0, 1.0};
  material->uniform.metallic           = 1.0;
  material->uniform.roughness          = 1.0;
  material->uniform.emissive_factor    = (vec4_t){1.0, 1.0, 1.0, 1.0};
  material->uniform.has_normal_texture = 1.0f;

  material->albedo_texture = options->albedo_texture;
  if (material->albedo_texture == NULL) {
    material->albedo_texture = &g_eng.white_texture;
  }

  material->normal_texture = options->normal_texture;
  if (material->normal_texture == NULL) {
    material->uniform.has_normal_texture = 0.0f;
    material->normal_texture             = &g_eng.white_texture;
  }

  material->metallic_roughness_texture = options->metallic_roughness_texture;
  if (material->metallic_roughness_texture == NULL) {
    material->metallic_roughness_texture = &g_eng.white_texture;
  }

  material->occlusion_texture = options->occlusion_texture;
  if (material->occlusion_texture == NULL) {
    material->occlusion_texture = &g_eng.white_texture;
  }

  material->emissive_texture = options->emissive_texture;
  if (material->emissive_texture == NULL) {
    material->emissive_texture = &g_eng.black_texture;
  }

  return material;
}

void eg_pbr_material_asset_inspect(
    eg_pbr_material_asset_t *material, eg_inspector_t *inspector) {
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

void eg_pbr_material_asset_destroy(eg_pbr_material_asset_t *material) {}

void eg_pbr_material_asset_bind(
    eg_pbr_material_asset_t *material,
    re_cmd_buffer_t *cmd_buffer,
    re_pipeline_t *pipeline,
    uint32_t set) {
  re_cmd_bind_image(cmd_buffer, set, 0, material->albedo_texture);
  re_cmd_bind_image(cmd_buffer, set, 1, material->normal_texture);
  re_cmd_bind_image(cmd_buffer, set, 2, material->metallic_roughness_texture);
  re_cmd_bind_image(cmd_buffer, set, 3, material->occlusion_texture);
  re_cmd_bind_image(cmd_buffer, set, 4, material->emissive_texture);

  void *mapping =
      re_cmd_bind_uniform(cmd_buffer, set, 5, sizeof(material->uniform));
  memcpy(mapping, &material->uniform, sizeof(material->uniform));

  re_cmd_bind_descriptor_set(cmd_buffer, pipeline, set);
}
