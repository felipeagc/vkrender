#include "pbr_material_asset.h"

#include "../asset_manager.h"
#include "../deserializer.h"
#include "../engine.h"
#include "../imgui.h"
#include "../serializer.h"
#include "image_asset.h"
#include <renderer/buffer.h>
#include <renderer/image.h>
#include <renderer/pipeline.h>
#include <string.h>

void eg_pbr_material_asset_init(
    eg_pbr_material_asset_t *material,
    eg_pbr_material_asset_options_t *options) {
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

enum {
  PROP_UNIFORM,
  PROP_ALBEDO,
  PROP_NORMAL,
  PROP_METALLIC_ROUGHNESS,
  PROP_OCCLUSION,
  PROP_EMISSIVE,
  PROP_MAX,
};

void eg_pbr_material_asset_serialize(
    eg_pbr_material_asset_t *material, eg_serializer_t *serializer) {
  eg_serializer_append_u32(serializer, PROP_MAX);

  eg_serializer_append_u32(serializer, PROP_UNIFORM);
  eg_serializer_append(
      serializer, &material->uniform, sizeof(material->uniform));

  eg_serializer_append_u32(serializer, PROP_ALBEDO);
  eg_asset_uid_t albedo_uid = EG_NULL_ASSET_UID;
  if (material->albedo_texture)
    albedo_uid = material->albedo_texture->asset.uid;
  eg_serializer_append_u32(serializer, albedo_uid);

  eg_serializer_append_u32(serializer, PROP_NORMAL);
  eg_asset_uid_t normal_uid = EG_NULL_ASSET_UID;
  if (material->normal_texture)
    normal_uid = material->normal_texture->asset.uid;
  eg_serializer_append_u32(serializer, normal_uid);

  eg_serializer_append_u32(serializer, PROP_METALLIC_ROUGHNESS);
  eg_asset_uid_t metallic_roughness_uid = EG_NULL_ASSET_UID;
  if (material->metallic_roughness_texture)
    metallic_roughness_uid = material->metallic_roughness_texture->asset.uid;
  eg_serializer_append_u32(serializer, metallic_roughness_uid);

  eg_serializer_append_u32(serializer, PROP_OCCLUSION);
  eg_asset_uid_t occlusion_uid = EG_NULL_ASSET_UID;
  if (material->occlusion_texture)
    occlusion_uid = material->occlusion_texture->asset.uid;
  eg_serializer_append_u32(serializer, occlusion_uid);

  eg_serializer_append_u32(serializer, PROP_EMISSIVE);
  eg_asset_uid_t emissive_uid = EG_NULL_ASSET_UID;
  if (material->emissive_texture)
    emissive_uid = material->emissive_texture->asset.uid;
  eg_serializer_append_u32(serializer, emissive_uid);
}

void eg_pbr_material_asset_deserialize(
    eg_pbr_material_asset_t *material, eg_deserializer_t *deserializer) {
  uint32_t prop_count = eg_deserializer_read_u32(deserializer);

  eg_pbr_material_asset_options_t options = {0};
  eg_pbr_material_uniform_t uniform       = {0};

  for (uint32_t i = 0; i < prop_count; i++) {
    uint32_t prop = eg_deserializer_read_u32(deserializer);

    switch (prop) {
    case PROP_UNIFORM: {
      eg_deserializer_read(deserializer, &uniform, sizeof(uniform));
      break;
    }
    case PROP_ALBEDO: {
      eg_asset_uid_t uid = eg_deserializer_read_u32(deserializer);
      options.albedo_texture =
          eg_asset_manager_get_by_uid(deserializer->asset_manager, uid);
      break;
    }
    case PROP_NORMAL: {
      eg_asset_uid_t uid = eg_deserializer_read_u32(deserializer);
      options.normal_texture =
          eg_asset_manager_get_by_uid(deserializer->asset_manager, uid);
      break;
    }
    case PROP_METALLIC_ROUGHNESS: {
      eg_asset_uid_t uid = eg_deserializer_read_u32(deserializer);
      options.metallic_roughness_texture =
          eg_asset_manager_get_by_uid(deserializer->asset_manager, uid);
      break;
    }
    case PROP_OCCLUSION: {
      eg_asset_uid_t uid = eg_deserializer_read_u32(deserializer);
      options.occlusion_texture =
          eg_asset_manager_get_by_uid(deserializer->asset_manager, uid);
      break;
    }
    case PROP_EMISSIVE: {
      eg_asset_uid_t uid = eg_deserializer_read_u32(deserializer);
      options.emissive_texture =
          eg_asset_manager_get_by_uid(deserializer->asset_manager, uid);
      break;
    }
    default: break;
    }
  }

  eg_pbr_material_asset_init(material, &options);
  material->uniform = uniform;
}

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
