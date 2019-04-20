#pragma once

#include <stddef.h>

/*
  To add a new component type:
  1. Add it to the eg_component_type_t enum
  2. Add the EG_COMP macros for it
  3. Register it in world.c
  4. Initialize the components in world.c
 */

typedef void (*eg_component_destructor_t)(void *);

#define EG_COMP_TYPE(comp) EG_COMP_TYPE_##comp

#define EG__COMPS                                                              \
  E(eg_transform_component_t, eg_transform_component_destroy, "Transform")     \
  E(eg_mesh_component_t, eg_mesh_component_destroy, "Mesh")                    \
  E(eg_gltf_model_component_t, eg_gltf_model_component_destroy, "GLTF Model")

#define E(t, destructor, name) EG_COMP_TYPE(t),
typedef enum eg_component_type_t {
  EG__COMPS EG_COMP_TYPE_MAX
} eg_component_type_t;
#undef E

extern const size_t EG_COMP_SIZES[EG_COMP_TYPE_MAX];
extern const char *EG_COMP_NAMES[EG_COMP_TYPE_MAX];
extern const eg_component_destructor_t EG_COMP_DESTRUCTORS[EG_COMP_TYPE_MAX];
