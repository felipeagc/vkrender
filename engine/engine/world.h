#pragma once

#include "assets/environment_asset.h"
#include "camera.h"
#include "components/component_types.h"
#include "environment.h"
#include <fstd_bitset.h>

#define EG_MAX_ENTITIES 128

#define EG_COMP_ARRAY(world, comp)                                             \
  ((comp *)(world)->pools[EG_COMP_TYPE(comp)].data)

#define EG_COMP(world, comp, entity)                                           \
  (&((comp *)(world)->pools[EG_COMP_TYPE(comp)].data)[entity])

#define EG_HAS_COMP(world, comp, entity)                                       \
  fstd_bitset_at((world)->comp_masks[EG_COMP_TYPE(comp)].bytes, entity)

#define EG_ADD_COMP(world, comp, entity)                                       \
  eg_world_add_comp((world), EG_COMP_TYPE(comp), entity)

#define EG_REMOVE_COMP(world, comp, entity)                                    \
  eg_world_remove_comp((world), EG_COMP_TYPE(comp), entity)

#define EG_TAGS(world, entity) ((world)->tags[entity])

#define EG_HAS_TAG(world, entity, tag) ((world)->tags[entity] & (1UL << tag))

#define EG_ADD_TAG(world, entity, tag)                                         \
  do {                                                                         \
    assert(entity < EG_MAX_ENTITIES);                                          \
    ((world)->tags[entity] |= (1UL << tag));                                   \
  } while (0)

#define EG_REMOVE_TAG(world, entity, tag)                                      \
  do {                                                                         \
    assert(entity < EG_MAX_ENTITIES);                                          \
    ((world)->tags[entity] &= ~(1UL << tag));                                  \
  } while (0)

#define EG_SET_TAG(world, entity, tag, value)                                  \
  do {                                                                         \
    assert(entity < EG_MAX_ENTITIES);                                          \
    ((world)->tags[entity] ^=                                                  \
     (-(uint64_t)value ^ (world)->tags[entity]) & (1UL << tag));               \
  } while (0)

typedef uint32_t eg_entity_t;

typedef FSTD_BITSET(EG_MAX_ENTITIES) eg_entity_mask_t;

typedef struct eg_component_pool_t {
  uint8_t *data;
} eg_component_pool_t;

typedef struct eg_world_t {
  eg_camera_t camera;
  eg_environment_t environment;

  eg_component_pool_t pools[EG_COMP_TYPE_MAX];
  eg_entity_mask_t comp_masks[EG_COMP_TYPE_MAX];
  uint64_t tags[EG_MAX_ENTITIES];
} eg_world_t;

void eg_world_init(
    eg_world_t *world, eg_environment_asset_t *environment_asset);

eg_entity_t eg_world_add(eg_world_t *world);

void eg_world_remove(eg_world_t *world, eg_entity_t entity);

void *eg_world_add_comp(
    eg_world_t *world, eg_component_type_t comp, eg_entity_t entity);

void eg_world_remove_comp(
    eg_world_t *world, eg_component_type_t comp, eg_entity_t entity);

bool eg_world_has_any_comp(eg_world_t *world, eg_entity_t entity);

void eg_world_destroy(eg_world_t *world);
