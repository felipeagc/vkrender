#pragma once

#include <stddef.h>

/*
  To add a new asset type:
  1. Add it to the eg_asset_type_t enum
  2. Make sure to have the first member in the specific asset struct be an
     eg_asset_t
  3. Add the proper #defines in asset_types.h
  3. Register the destructors in asset_types.c
 */

typedef void (*eg_asset_destructor_t)(void *);

#define EG_ASSET_TYPE(type) EG_ASSET_TYPE_##type

#define EG_ASSET_TYPE_eg_pipeline_asset_t EG_PIPELINE_ASSET_TYPE
#define EG_ASSET_TYPE_eg_environment_asset_t EG_ENVIRONMENT_ASSET_TYPE
#define EG_ASSET_TYPE_eg_mesh_asset_t EG_MESH_ASSET_TYPE
#define EG_ASSET_TYPE_eg_pbr_material_asset_t EG_PBR_MATERIAL_ASSET_TYPE
#define EG_ASSET_TYPE_eg_gltf_model_asset_t EG_GLTF_MODEL_ASSET_TYPE

typedef enum eg_asset_type_t {
  EG_PIPELINE_ASSET_TYPE,
  EG_ENVIRONMENT_ASSET_TYPE,
  EG_MESH_ASSET_TYPE,
  EG_PBR_MATERIAL_ASSET_TYPE,
  EG_GLTF_MODEL_ASSET_TYPE,
  EG_ASSET_TYPE_COUNT,
} eg_asset_type_t;

extern eg_asset_destructor_t eg_asset_destructors[EG_ASSET_TYPE_COUNT];

typedef struct eg_asset_t {
  eg_asset_type_t type;

  // This string's lifetime is managed by the
  // asset manager's hash map, so no need to free it.
  const char *name;
} eg_asset_t;
