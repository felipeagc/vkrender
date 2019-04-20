#pragma once

#include <stddef.h>

/*
  To add a new component type add it to the EG__COMPS macro
 */

typedef void (*eg_component_destructor_t)(void *);

#define EG_COMP_TYPE(comp) EG_COMP_TYPE_##comp

#define EG__COMPS                                                              \
  E(eg_transform_component_t, eg_transform_component_destroy, "Transform")     \
  E(eg_mesh_component_t, eg_mesh_component_destroy, "Mesh")                    \
  E(eg_gltf_model_component_t, eg_gltf_model_component_destroy, "GLTF Model")

#define EG__TAGS E(EG_TAG_HIDDEN, "Hidden")

#define E(t, destructor, name) EG_COMP_TYPE(t),
typedef enum eg_component_type_t {
  EG__COMPS EG_COMP_TYPE_MAX
} eg_component_type_t;
#undef E

extern const size_t EG_COMP_SIZES[EG_COMP_TYPE_MAX];
extern const char *EG_COMP_NAMES[EG_COMP_TYPE_MAX];
extern const eg_component_destructor_t EG_COMP_DESTRUCTORS[EG_COMP_TYPE_MAX];

#define E(enum_name, name) enum_name,
typedef enum eg_tag_t { EG__TAGS EG_TAG_MAX } eg_tag_t;
#undef E

extern const char *EG_TAG_NAMES[EG_TAG_MAX];
