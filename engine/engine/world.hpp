#pragma once

#include "camera.hpp"
#include "environment.hpp"
#include "mesh.hpp"
#include <util/bitset.h>

#define EG_MAX_ENTITIES 128

#define EG_COMP_INDEX(comp) EG_COMP_INDEX_##comp
#define EG_COMP_INIT(comp) EG_COMP_INIT_##comp
#define EG_COMP_DESTROY(comp) EG_COMP_DESTROY_##comp

#define EG_COMP_INDEX_eg_mesh_t EG_MESH_INDEX
#define EG_COMP_INIT_eg_mesh_t eg_mesh_init
#define EG_COMP_DESTROY_eg_mesh_t eg_mesh_destroy

#define EG_GET_ALL_COMPS(world, comp_type)                                     \
  ((comp_type *)(world)->entities.components[EG_COMP_INDEX(comp_type)].array)

#define EG_GET_COMP(world, comp_type, entity)                                  \
  (&EG_GET_ALL_COMPS(world, comp_type)[entity])

#define EG_FOR_EVERY_COMP(world, comp_type, comp_var, ...)                     \
  for (uint32_t e = 0; e < EG_MAX_ENTITIES; e++) {                             \
    if (eg_world_has_comp(world, e, EG_COMP_INDEX(comp_type))) {               \
      comp_type *comp_var = EG_GET_COMP(world, comp_type, e);                  \
      __VA_ARGS__                                                              \
    }                                                                          \
  }

#define EG_ADD_COMP(world, entity, comp_type)                                  \
  {                                                                            \
    eg_entity_t e = entity;                                                    \
    eg_world_add_comp(world, e, EG_COMP_INDEX(comp_type));                     \
  }

#define EG_ADD_COMP_INIT(world, entity, comp_type, ...)                        \
  {                                                                            \
    eg_entity_t e = entity;                                                    \
    EG_COMP_INIT(comp_type)                                                    \
    (EG_GET_COMP(world, comp_type, e), __VA_ARGS__);                           \
    eg_world_add_comp(world, e, EG_COMP_INDEX(comp_type));                     \
  }

typedef enum {
  EG_MESH_INDEX,
  EG_COMP_COUNT,
} eg_component_index_t;

typedef uint32_t eg_entity_t;

typedef UT_BITSET(EG_MAX_ENTITIES) eg_entities_bitset_t;

typedef struct {
  uint8_t bytes[EG_MAX_ENTITIES / 8];
} eg_bitset_t;

typedef struct {
  void *array;
  eg_entities_bitset_t bitset;
} eg_component_bundle_t;

typedef struct eg_entities_t {
  eg_component_bundle_t components[EG_COMP_COUNT];
} eg_entities_t;

typedef struct eg_world_t {
  eg_camera_t camera;
  eg_environment_t environment;

  eg_entities_t entities;
} eg_world_t;

void eg_world_init(
    eg_world_t *world, eg_environment_asset_t *environment_asset);

eg_entity_t eg_world_add_entity(eg_world_t *world);

void eg_world_remove_entity(eg_world_t *world, eg_entity_t entity);

void eg_world_add_comp(
    eg_world_t *world, eg_entity_t entity, eg_component_index_t comp);

bool eg_world_has_comp(
    eg_world_t *world, eg_entity_t entity, eg_component_index_t comp);

void eg_world_remove_comp(
    eg_world_t *world, eg_entity_t entity, eg_component_index_t comp);

void eg_world_destroy(eg_world_t *world);
