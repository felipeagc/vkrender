#pragma once

#include <stddef.h>

typedef void (*eg_component_destructor_t)(void *);

typedef enum {
  EG_MESH_COMPONENT_TYPE,
  EG_COMPONENT_TYPE_COUNT,
} eg_component_type_t;

extern eg_component_destructor_t
    eg_component_destructors[EG_COMPONENT_TYPE_COUNT];
extern size_t eg_component_sizes[EG_COMPONENT_TYPE_COUNT];

#define EG_REGISTER_COMP(comp)                                                 \
  {                                                                            \
    eg_component_sizes[EG_COMP_TYPE(comp)] = sizeof(comp);                     \
    eg_component_destructors[EG_COMP_TYPE(comp)] =                             \
        (eg_component_destructor_t)EG_COMP_DESTROY(comp);                      \
  }

#define EG_COMP_TYPE(comp) EG_COMP_TYPE_##comp
#define EG_COMP_INIT(comp) EG_COMP_INIT_##comp
#define EG_COMP_DESTROY(comp) EG_COMP_DESTROY_##comp

#define EG_COMP_TYPE_eg_mesh_component_t EG_MESH_COMPONENT_TYPE
#define EG_COMP_INIT_eg_mesh_component_t eg_mesh_component_init
#define EG_COMP_DESTROY_eg_mesh_component_t eg_mesh_component_destroy
