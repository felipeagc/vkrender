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

typedef enum eg_component_type_t {
  EG_TRANSFORM_COMPONENT_TYPE,
  EG_MESH_COMPONENT_TYPE,
  EG_GLTF_MODEL_COMPONENT_TYPE,
  EG_COMPONENT_TYPE_COUNT,
} eg_component_type_t;

extern eg_component_destructor_t
    eg_component_destructors[EG_COMPONENT_TYPE_COUNT];
extern size_t eg_component_sizes[EG_COMPONENT_TYPE_COUNT];

#define EG_COMP_TYPE(comp) EG_COMP_TYPE_##comp
#define EG_COMP_DESTROY(comp) EG_COMP_DESTROY_##comp

#define EG_COMP_TYPE_eg_transform_component_t EG_TRANSFORM_COMPONENT_TYPE
#define EG_COMP_DESTROY_eg_transform_component_t eg_transform_component_destroy

#define EG_COMP_TYPE_eg_mesh_component_t EG_MESH_COMPONENT_TYPE
#define EG_COMP_DESTROY_eg_mesh_component_t eg_mesh_component_destroy

#define EG_COMP_TYPE_eg_gltf_model_component_t EG_GLTF_MODEL_COMPONENT_TYPE
#define EG_COMP_DESTROY_eg_gltf_model_component_t                              \
  eg_gltf_model_component_destroy

#define EG_REGISTER_COMP(comp)                                                 \
  {                                                                            \
    eg_component_sizes[EG_COMP_TYPE(comp)] = sizeof(comp);                     \
    eg_component_destructors[EG_COMP_TYPE(comp)] =                             \
        (eg_component_destructor_t)EG_COMP_DESTROY(comp);                      \
  }
