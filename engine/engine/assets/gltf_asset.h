#pragma once

#include "asset_types.h"
#include "pbr_material_asset.h"
#include <gmath.h>
#include <renderer/buffer.h>
#include <renderer/image.h>

typedef struct eg_gltf_asset_dimensions_t {
  vec3_t min;
  vec3_t max;
  vec3_t size;
  vec3_t center;
  float radius;
} eg_gltf_asset_dimensions_t;

typedef struct eg_gltf_asset_material_t {
  eg_pbr_material_uniform_t uniform;
  re_image_t *albedo_texture;
  re_image_t *normal_texture;
  re_image_t *metallic_roughness_texture;
  re_image_t *occlusion_texture;
  re_image_t *emissive_texture;
} eg_gltf_asset_material_t;

typedef struct eg_gltf_asset_primitive_t {
  uint32_t first_index;
  uint32_t index_count;
  eg_gltf_asset_material_t *material;

  eg_gltf_asset_dimensions_t dimensions;
} eg_gltf_asset_primitive_t;

typedef struct eg_gltf_asset_mesh_t {
  eg_gltf_asset_primitive_t *primitives;
  uint32_t primitive_count;

  mat4_t matrix;
} eg_gltf_asset_mesh_t;

typedef struct eg_gltf_asset_node_t {
  struct eg_gltf_asset_node_t *parent;
  struct eg_gltf_asset_node_t **children;
  uint32_t children_count;

  mat4_t matrix;
  char *name;
  eg_gltf_asset_mesh_t *mesh;

  vec3_t translation;
  vec3_t scale;
  quat_t rotation;
} eg_gltf_asset_node_t;

typedef struct eg_gltf_asset_options_t {
  const char *path;
  bool flip_uvs;
} eg_gltf_asset_options_t;

typedef struct eg_gltf_asset_t {
  eg_asset_t asset;

  re_buffer_t vertex_buffer;
  re_buffer_t index_buffer;
  uint32_t vertex_count;
  uint32_t index_count;

  re_image_t *images;
  uint32_t image_count;

  eg_gltf_asset_material_t *materials;
  uint32_t material_count;

  eg_gltf_asset_node_t *nodes;
  uint32_t node_count;

  eg_gltf_asset_mesh_t *meshes;
  uint32_t mesh_count;

  eg_gltf_asset_dimensions_t dimensions;
} eg_gltf_asset_t;

/*
 * Required asset functions
 */
eg_gltf_asset_t *eg_gltf_asset_create(
    eg_asset_manager_t *asset_manager, eg_gltf_asset_options_t *options);

void eg_gltf_asset_inspect(eg_gltf_asset_t *model, eg_inspector_t *inspector);

void eg_gltf_asset_destroy(eg_gltf_asset_t *model);
