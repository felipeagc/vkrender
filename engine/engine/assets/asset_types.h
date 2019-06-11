#pragma once

#include <stddef.h>
#include <stdint.h>

/*
  To add a new asset type:
  1. Add it to the eg_asset_type_t enum
  2. Make sure to have the first member in the specific asset struct be an
     eg_asset_t
  3. Add the proper #defines in asset_types.h
  3. Register the destructors in asset_types.c
 */

typedef struct eg_inspector_t eg_inspector_t;

extern const char *const EG_DEFAULT_ASSET_NAME;

typedef void (*eg_asset_initializer_t)(void *);
typedef void (*eg_asset_inspector_t)(void *, eg_inspector_t *inspector);
typedef void (*eg_asset_destructor_t)(void *);

#define EG_ASSET_TYPE(type) EG_ASSET_TYPE_##type
#define EG_ASSET_NAME(type) EG_ASSET_NAMES[EG_ASSET_TYPE(type)]

#define EG__ASSETS                                                             \
  E(eg_pipeline_asset_t,                                                       \
    eg_pipeline_asset_default,                                                 \
    eg_pipeline_asset_inspect,                                                 \
    eg_pipeline_asset_destroy,                                                 \
    "Pipeline")                                                                \
  E(eg_image_asset_t,                                                          \
    eg_image_asset_default,                                                    \
    eg_image_asset_inspect,                                                    \
    eg_image_asset_destroy,                                                    \
    "Image")                                                                   \
  E(eg_mesh_asset_t,                                                           \
    eg_mesh_asset_default,                                                     \
    eg_mesh_asset_inspect,                                                     \
    eg_mesh_asset_destroy,                                                     \
    "Mesh")                                                                    \
  E(eg_pbr_material_asset_t,                                                   \
    eg_pbr_material_asset_default,                                             \
    eg_pbr_material_asset_inspect,                                             \
    eg_pbr_material_asset_destroy,                                             \
    "PBR material")                                                            \
  E(eg_gltf_asset_t,                                                           \
    eg_gltf_asset_default,                                                     \
    eg_gltf_asset_inspect,                                                     \
    eg_gltf_asset_destroy,                                                     \
    "GLTF model")

#define E(t, initializer, inspector, destructor, name) EG_ASSET_TYPE(t),
typedef enum eg_asset_type_t { EG__ASSETS EG_ASSET_TYPE_MAX } eg_asset_type_t;
#undef E

extern const size_t EG_ASSET_SIZES[EG_ASSET_TYPE_MAX];
extern const char *EG_ASSET_NAMES[EG_ASSET_TYPE_MAX];
/* extern const eg_asset_initializer_t EG_ASSET_INITIALIZERS[EG_ASSET_TYPE_MAX];
 */
extern const eg_asset_inspector_t EG_ASSET_INSPECTORS[EG_ASSET_TYPE_MAX];
extern const eg_asset_destructor_t EG_ASSET_DESTRUCTORS[EG_ASSET_TYPE_MAX];

typedef struct eg_asset_t {
  eg_asset_type_t type;
  uint32_t index;
  char *name;
} eg_asset_t;

void eg_asset_set_name(eg_asset_t *asset, const char *name);

const char *eg_asset_get_name(eg_asset_t *asset);
