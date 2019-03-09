#pragma once

#include "asset_types.h"
#include <gmath.h>
#include <renderer/common.h>
#include <vulkan/vulkan.h>

typedef struct re_texture_t re_texture_t;
typedef struct re_window_t re_window_t;
typedef struct re_pipeline_t re_pipeline_t;

typedef struct eg_pbr_material_uniform_t {
  vec4_t base_color_factor;
  float metallic;
  float roughness;
  vec4_t emissive_factor;
  float has_normal_texture;
} eg_pbr_material_uniform_t;

typedef struct eg_pbr_material_asset_t {
  eg_asset_t asset;
  eg_pbr_material_uniform_t uniform;
  VkDescriptorSet descriptor_sets[RE_MAX_FRAMES_IN_FLIGHT];
} eg_pbr_material_asset_t;

void eg_pbr_material_asset_init(
    eg_pbr_material_asset_t *material,
    struct re_texture_t *albedo_texture,
    struct re_texture_t *normal_texture,
    struct re_texture_t *metallic_roughness_texture,
    struct re_texture_t *occlusion_texture,
    struct re_texture_t *emissive_texture);

void eg_pbr_material_asset_bind(
    eg_pbr_material_asset_t *material,
    struct re_window_t *window,
    struct re_pipeline_t *pipeline,
    uint32_t set_index);

void eg_pbr_material_asset_destroy(eg_pbr_material_asset_t *material);