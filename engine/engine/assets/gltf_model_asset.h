#pragma once

#include "asset_types.h"
#include "pbr_material_asset.h"
#include <gmath.h>
#include <renderer/buffer.h>
#include <renderer/image.h>

typedef struct re_window_t re_window_t;

typedef struct eg_gltf_model_asset_dimensions {
  vec3_t min;
  vec3_t max;
  vec3_t size;
  vec3_t center;
  float radius;
} eg_gltf_model_asset_dimensions_t;

typedef struct eg_gltf_model_asset_material {
  eg_pbr_material_uniform_t uniform;
  VkDescriptorSet descriptor_sets[RE_MAX_FRAMES_IN_FLIGHT];
} eg_gltf_model_asset_material_t;

typedef struct eg_gltf_model_asset_primitive {
  uint32_t first_index;
  uint32_t index_count;
  eg_gltf_model_asset_material_t *material;

  eg_gltf_model_asset_dimensions_t dimensions;
} eg_gltf_model_asset_primitive_t;

typedef struct eg_gltf_model_asset_mesh {
  eg_gltf_model_asset_primitive_t *primitives;
  uint32_t primitive_count;

  struct {
    mat4_t matrix;
  } ubo;

  re_buffer_t uniform_buffers[RE_MAX_FRAMES_IN_FLIGHT];
  void *mappings[RE_MAX_FRAMES_IN_FLIGHT];
  VkDescriptorSet descriptor_sets[RE_MAX_FRAMES_IN_FLIGHT];
} eg_gltf_model_asset_mesh_t;

typedef struct eg_gltf_model_asset_node {
  struct eg_gltf_model_asset_node *parent;
  struct eg_gltf_model_asset_node **children;
  uint32_t children_count;

  mat4_t matrix;
  char *name;
  eg_gltf_model_asset_mesh_t *mesh;

  vec3_t translation;
  vec3_t scale;
  quat_t rotation;
} eg_gltf_model_asset_node_t;

typedef struct eg_gltf_model_asset {
  eg_asset_t asset;

  re_buffer_t vertex_buffer;
  re_buffer_t index_buffer;
  uint32_t index_count;

  re_image_t *images;
  uint32_t image_count;

  eg_gltf_model_asset_material_t *materials;
  uint32_t material_count;

  eg_gltf_model_asset_node_t *nodes;
  uint32_t node_count;

  eg_gltf_model_asset_mesh_t *meshes;
  uint32_t mesh_count;

  eg_gltf_model_asset_dimensions_t dimensions;
} eg_gltf_model_asset_t;

void eg_gltf_model_asset_init(eg_gltf_model_asset_t *model, const char *path);

void eg_gltf_model_asset_draw(
    eg_gltf_model_asset_t *model, struct re_window_t *window);

void eg_gltf_model_asset_destroy(eg_gltf_model_asset_t *model);
