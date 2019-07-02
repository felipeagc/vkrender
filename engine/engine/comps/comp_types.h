#pragma once

#include <stddef.h>

/*
  To add a new component type add it to the EG__COMPS macro
 */

typedef struct eg_inspector_t eg_inspector_t;
typedef struct eg_serializer_t eg_serializer_t;
typedef struct eg_deserializer_t eg_deserializer_t;

typedef void (*eg_comp_destructor_t)(void *);
typedef void (*eg_comp_initializer_t)(void *);
typedef void (*eg_comp_inspector_t)(void *, eg_inspector_t *);
typedef void (*eg_comp_serializer_t)(void *, eg_serializer_t *);
typedef void (*eg_comp_deserializer_t)(void *, eg_deserializer_t *);

#define EG_COMP_TYPE(comp) EG_COMP_TYPE_##comp
#define EG_COMP_NAME(comp) EG_COMP_NAMES[EG_COMP_TYPE(comp)]

#define EG__COMPS                                                              \
  E(eg_transform_comp_t,                                                       \
    eg_transform_comp_default,                                                 \
    eg_transform_comp_inspect,                                                 \
    eg_transform_comp_destroy,                                                 \
    eg_transform_comp_serialize,                                               \
    eg_transform_comp_deserialize,                                             \
    "Transform")                                                               \
  E(eg_point_light_comp_t,                                                     \
    eg_point_light_comp_default,                                               \
    eg_point_light_comp_inspect,                                               \
    eg_point_light_comp_destroy,                                               \
    eg_point_light_comp_serialize,                                             \
    eg_point_light_comp_deserialize,                                           \
    "Point Light")                                                             \
  E(eg_renderable_comp_t,                                                      \
    eg_renderable_comp_default,                                                \
    eg_renderable_comp_inspect,                                                \
    eg_renderable_comp_destroy,                                                \
    eg_renderable_comp_serialize,                                              \
    eg_renderable_comp_deserialize,                                            \
    "Renderable")                                                              \
  E(eg_mesh_comp_t,                                                            \
    eg_mesh_comp_default,                                                      \
    eg_mesh_comp_inspect,                                                      \
    eg_mesh_comp_destroy,                                                      \
    eg_mesh_comp_serialize,                                                    \
    eg_mesh_comp_deserialize,                                                  \
    "Mesh")                                                                    \
  E(eg_gltf_comp_t,                                                            \
    eg_gltf_comp_default,                                                      \
    eg_gltf_comp_inspect,                                                      \
    eg_gltf_comp_destroy,                                                      \
    eg_gltf_comp_serialize,                                                    \
    eg_gltf_comp_deserialize,                                                  \
    "GLTF Model")                                                              \
  E(eg_terrain_comp_t,                                                         \
    eg_terrain_comp_default,                                                   \
    eg_terrain_comp_inspect,                                                   \
    eg_terrain_comp_destroy,                                                   \
    eg_terrain_comp_serialize,                                                 \
    eg_terrain_comp_deserialize,                                               \
    "Terrain")

#define EG__TAGS E(EG_TAG_HIDDEN, "Hidden")

#define E(                                                                     \
    t, initializer, inspector, destructor, serializer, deserializer, name)     \
  EG_COMP_TYPE(t),
typedef enum eg_comp_type_t { EG__COMPS EG_COMP_TYPE_MAX } eg_comp_type_t;
#undef E

extern const size_t EG_COMP_SIZES[EG_COMP_TYPE_MAX];
extern const char *EG_COMP_NAMES[EG_COMP_TYPE_MAX];
extern const eg_comp_initializer_t EG_COMP_INITIALIZERS[EG_COMP_TYPE_MAX];
extern const eg_comp_inspector_t EG_COMP_INSPECTORS[EG_COMP_TYPE_MAX];
extern const eg_comp_destructor_t EG_COMP_DESTRUCTORS[EG_COMP_TYPE_MAX];
extern const eg_comp_serializer_t EG_COMP_SERIALIZERS[EG_COMP_TYPE_MAX];
extern const eg_comp_deserializer_t EG_COMP_DESERIALIZERS[EG_COMP_TYPE_MAX];

#define E(enum_name, name) enum_name,
typedef enum eg_tag_t { EG__TAGS EG_TAG_MAX } eg_tag_t;
#undef E

extern const char *EG_TAG_NAMES[EG_TAG_MAX];
