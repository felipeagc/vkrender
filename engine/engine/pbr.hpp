#pragma once

#include <renderer/buffer.hpp>
#include <renderer/common.hpp>
#include <renderer/context.hpp>
#include <renderer/resource_manager.hpp>
#include <renderer/util.hpp>
#include <vkm/vkm.h>

typedef struct eg_pbr_model_uniform_t {
  mat4_t transform;
} eg_pbr_model_uniform_t;

typedef struct eg_pbr_model_t {
  eg_pbr_model_uniform_t uniform;
  re_buffer_t buffers[RE_MAX_FRAMES_IN_FLIGHT];
  void *mappings[RE_MAX_FRAMES_IN_FLIGHT];
  re_resource_set_t resource_sets[RE_MAX_FRAMES_IN_FLIGHT];
} eg_pbr_model_t;

void eg_pbr_model_init(eg_pbr_model_t *model, mat4_t transform);

void eg_pbr_model_update_uniform(
    eg_pbr_model_t *model, struct re_window_t *window);

void eg_pbr_model_bind(
    eg_pbr_model_t *model,
    struct re_window_t *window,
    struct re_pipeline_t *pipeline,
    uint32_t set_index);

void eg_pbr_model_destroy(eg_pbr_model_t *model);

typedef struct eg_pbr_material_uniform_t {
  vec4_t base_color_factor;
  float metallic;
  float roughness;
  vec4_t emissive_factor;
  float has_normal_texture;
} eg_pbr_material_uniform_t;

typedef struct eg_pbr_material_t {
  eg_pbr_material_uniform_t uniform;
  re_resource_set_t resource_sets[RE_MAX_FRAMES_IN_FLIGHT];
} eg_pbr_material_t;

void eg_pbr_material_init(
    eg_pbr_material_t *material,
    struct re_texture_t *albedo_texture,
    struct re_texture_t *normal_texture,
    struct re_texture_t *metallic_roughness_texture,
    struct re_texture_t *occlusion_texture,
    struct re_texture_t *emissive_texture);

void eg_pbr_material_bind(
    eg_pbr_material_t *material,
    struct re_window_t *window,
    struct re_pipeline_t *pipeline,
    uint32_t set_index);

void eg_pbr_material_destroy(eg_pbr_material_t *material);
