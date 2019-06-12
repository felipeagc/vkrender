#pragma once

#include "asset_types.h"
#include <gmath.h>

typedef struct re_cmd_buffer_t re_cmd_buffer_t;
typedef struct re_pipeline_t re_pipeline_t;
typedef struct eg_image_asset_t eg_image_asset_t;

typedef struct eg_pbr_material_uniform_t {
  vec4_t base_color_factor;
  float metallic;
  float roughness;
  vec4_t emissive_factor;
  uint32_t has_normal_texture;
} eg_pbr_material_uniform_t;

typedef struct eg_pbr_material_asset_options_t {
  eg_image_asset_t *albedo_texture;
  eg_image_asset_t *normal_texture;
  eg_image_asset_t *metallic_roughness_texture;
  eg_image_asset_t *occlusion_texture;
  eg_image_asset_t *emissive_texture;
} eg_pbr_material_asset_options_t;

typedef struct eg_pbr_material_asset_t {
  eg_asset_t asset;
  eg_pbr_material_uniform_t uniform;
  eg_image_asset_t *albedo_texture;
  eg_image_asset_t *normal_texture;
  eg_image_asset_t *metallic_roughness_texture;
  eg_image_asset_t *occlusion_texture;
  eg_image_asset_t *emissive_texture;
} eg_pbr_material_asset_t;

/*
 * Required asset functions
 */
eg_pbr_material_asset_t *eg_pbr_material_asset_create(
    eg_asset_manager_t *asset_manager,
    eg_pbr_material_asset_options_t *options);

void eg_pbr_material_asset_inspect(
    eg_pbr_material_asset_t *material, eg_inspector_t *inspector);

void eg_pbr_material_asset_destroy(eg_pbr_material_asset_t *material);

/*
 * Specific functions
 */
void eg_pbr_material_asset_bind(
    eg_pbr_material_asset_t *material,
    re_cmd_buffer_t *cmd_buffer,
    struct re_pipeline_t *pipeline,
    uint32_t set_index);
