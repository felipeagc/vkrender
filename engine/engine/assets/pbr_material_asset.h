#pragma once

#include "asset_types.h"
#include <gmath.h>
#include <renderer/buffer.h>

typedef struct re_image_t re_image_t;
typedef struct re_pipeline_t re_pipeline_t;

typedef struct eg_pbr_material_uniform_t {
  vec4_t base_color_factor;
  float metallic;
  float roughness;
  vec4_t emissive_factor;
  float has_normal_texture;
} eg_pbr_material_uniform_t;

typedef struct eg_pbr_material_asset_options_t {
  re_image_t *albedo_texture;
  re_image_t *normal_texture;
  re_image_t *metallic_roughness_texture;
  re_image_t *occlusion_texture;
  re_image_t *emissive_texture;
} eg_pbr_material_asset_options_t;

typedef struct eg_pbr_material_asset_t {
  eg_asset_t asset;
  eg_pbr_material_uniform_t uniform;
  re_image_t *albedo_texture;
  re_image_t *normal_texture;
  re_image_t *metallic_roughness_texture;
  re_image_t *occlusion_texture;
  re_image_t *emissive_texture;
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
