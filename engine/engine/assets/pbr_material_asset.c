#include "pbr_material_asset.h"

#include "../asset_manager.h"
#include "../engine.h"
#include "../imgui.h"
#include "image_asset.h"
#include <renderer/buffer.h>
#include <renderer/image.h>
#include <renderer/pipeline.h>
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
  material->uniform.has_normal_texture = 0;

  material->albedo_texture             = options->albedo_texture;
  material->normal_texture             = options->normal_texture;
  material->occlusion_texture          = options->occlusion_texture;
  material->metallic_roughness_texture = options->metallic_roughness_texture;
  material->emissive_texture           = options->emissive_texture;

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
  re_image_t *albedo             = &g_eng.white_texture;
  re_image_t *normal             = &g_eng.white_texture;
  re_image_t *metallic_roughness = &g_eng.white_texture;
  re_image_t *occlusion          = &g_eng.white_texture;
  re_image_t *emissive           = &g_eng.black_texture;

  if (material->albedo_texture != NULL)
    albedo = &material->albedo_texture->image;
  if (material->normal_texture != NULL)
    normal = &material->normal_texture->image;
  if (material->metallic_roughness_texture != NULL)
    metallic_roughness = &material->metallic_roughness_texture->image;
  if (material->occlusion_texture != NULL)
    occlusion = &material->occlusion_texture->image;
  if (material->emissive_texture != NULL)
    emissive = &material->emissive_texture->image;

  re_cmd_bind_image(cmd_buffer, set, 0, albedo);
  re_cmd_bind_image(cmd_buffer, set, 1, normal);
  re_cmd_bind_image(cmd_buffer, set, 2, metallic_roughness);
  re_cmd_bind_image(cmd_buffer, set, 3, occlusion);
  re_cmd_bind_image(cmd_buffer, set, 4, emissive);

  material->uniform.has_normal_texture =
      (material->normal_texture != NULL) ? 1 : 0;

  void *mapping =
      re_cmd_bind_uniform(cmd_buffer, set, 5, sizeof(material->uniform));
  memcpy(mapping, &material->uniform, sizeof(material->uniform));

  re_cmd_bind_descriptor_set(cmd_buffer, pipeline, set);
}
