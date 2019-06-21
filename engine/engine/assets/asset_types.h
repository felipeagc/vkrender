#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct eg_inspector_t eg_inspector_t;
typedef struct eg_asset_manager_t eg_asset_manager_t;
typedef struct eg_serializer_t eg_serializer_t;

extern const char *const EG_DEFAULT_ASSET_NAME;

typedef void *(*eg_asset_constructor_t)(eg_asset_manager_t *, void *);
typedef void (*eg_asset_inspector_t)(void *, eg_inspector_t *inspector);
typedef void (*eg_asset_destructor_t)(void *);
typedef void (*eg_asset_serializer_t)(void *, eg_serializer_t *);

#define EG_ASSET_TYPE(type) EG_ASSET_TYPE_##type
#define EG_ASSET_NAME(type) EG_ASSET_NAMES[EG_ASSET_TYPE(type)]

#define EG__ASSETS                                                             \
  E(eg_pipeline_asset_t,                                                       \
    eg_pipeline_asset_create,                                                  \
    eg_pipeline_asset_inspect,                                                 \
    eg_pipeline_asset_destroy,                                                 \
    eg_pipeline_asset_serialize,                                               \
    eg_pipeline_asset_deserialize,                                             \
    "Pipeline")                                                                \
  E(eg_image_asset_t,                                                          \
    eg_image_asset_create,                                                     \
    eg_image_asset_inspect,                                                    \
    eg_image_asset_destroy,                                                    \
    eg_image_asset_serialize,                                                  \
    eg_image_asset_deserialize,                                                \
    "Image")                                                                   \
  E(eg_mesh_asset_t,                                                           \
    eg_mesh_asset_create,                                                      \
    eg_mesh_asset_inspect,                                                     \
    eg_mesh_asset_destroy,                                                     \
    eg_mesh_asset_serialize,                                                   \
    eg_mesh_asset_deserialize,                                                 \
    "Mesh")                                                                    \
  E(eg_pbr_material_asset_t,                                                   \
    eg_pbr_material_asset_create,                                              \
    eg_pbr_material_asset_inspect,                                             \
    eg_pbr_material_asset_destroy,                                             \
    eg_pbr_material_asset_serialize,                                           \
    eg_pbr_material_asset_deserialize,                                         \
    "PBR material")                                                            \
  E(eg_gltf_asset_t,                                                           \
    eg_gltf_asset_create,                                                      \
    eg_gltf_asset_inspect,                                                     \
    eg_gltf_asset_destroy,                                                     \
    eg_gltf_asset_serialize,                                                   \
    eg_gltf_asset_deserialize,                                                 \
    "GLTF model")

#define E(                                                                     \
    t, constructor, inspector, destructor, serializer, deserializer, name)     \
  EG_ASSET_TYPE(t),
typedef enum eg_asset_type_t { EG__ASSETS EG_ASSET_TYPE_MAX } eg_asset_type_t;
#undef E

extern const size_t EG_ASSET_SIZES[EG_ASSET_TYPE_MAX];
extern const char *EG_ASSET_NAMES[EG_ASSET_TYPE_MAX];
extern const eg_asset_constructor_t EG_ASSET_CONSTRUCTORS[EG_ASSET_TYPE_MAX];
extern const eg_asset_inspector_t EG_ASSET_INSPECTORS[EG_ASSET_TYPE_MAX];
extern const eg_asset_destructor_t EG_ASSET_DESTRUCTORS[EG_ASSET_TYPE_MAX];
extern const eg_asset_serializer_t EG_ASSET_SERIALIZERS[EG_ASSET_TYPE_MAX];
/* extern const eg_asset_deserializer_t
 * EG_ASSET_DESERIALIZERS[EG_ASSET_TYPE_MAX]; */

typedef struct eg_asset_t {
  eg_asset_type_t type;
  uint32_t uid; /* not associated with asset's position in the asset_manager */
  uint32_t index;
  char *name;
} eg_asset_t;

void eg_asset_set_name(eg_asset_t *asset, const char *name);

const char *eg_asset_get_name(eg_asset_t *asset);
