#pragma once

#include "comps/comp_types.h"
#include <stdbool.h>
#include <stdint.h>

#define EG_MAX_ENTITIES 128

#define EG_COMP_ARRAY(entity_manager, comp)                                    \
  ((comp *)(entity_manager)->pools[EG_COMP_TYPE(comp)].data)

#define EG_COMP_BY_ID(entity_manager, comp_id, entity)                         \
  (&(entity_manager)->pools[comp_id].data[entity * EG_COMP_SIZES[comp_id]])

#define EG_COMP(entity_manager, comp, entity)                                  \
  ((comp *)EG_COMP_BY_ID(entity_manager, EG_COMP_TYPE(comp), entity))

#define EG_HAS_COMP_ID(entity_manager, comp_id, entity)                        \
  ((entity_manager)->comp_masks[comp_id][entity])

#define EG_HAS_COMP(entity_manager, comp, entity)                              \
  EG_HAS_COMP_ID(entity_manager, EG_COMP_TYPE(comp), entity)

#define EG_ADD_COMP(entity_manager, comp, entity)                              \
  eg_comp_add((entity_manager), EG_COMP_TYPE(comp), entity)

#define EG_REMOVE_COMP(entity_manager, comp, entity)                           \
  eg_comp_remove((entity_manager), EG_COMP_TYPE(comp), entity)

#define EG_TAGS(entity_manager, entity) ((entity_manager)->tags[entity])

#define EG_HAS_TAG(entity_manager, entity, tag)                                \
  ((entity_manager)->tags[entity] & (1ULL << tag))

#define EG_ADD_TAG(entity_manager, entity, tag)                                \
  do {                                                                         \
    assert(entity < EG_MAX_ENTITIES);                                          \
    ((entity_manager)->tags[entity] |= (1ULL << tag));                         \
  } while (0)

#define EG_REMOVE_TAG(entity_manager, entity, tag)                             \
  do {                                                                         \
    assert(entity < EG_MAX_ENTITIES);                                          \
    ((entity_manager)->tags[entity] &= ~(1ULL << tag));                        \
  } while (0)

#define EG_SET_TAG(entity_manager, entity, tag, value)                         \
  do {                                                                         \
    assert(entity < EG_MAX_ENTITIES);                                          \
    ((entity_manager)->tags[entity] ^=                                         \
     (-(uint64_t)value ^ (entity_manager)->tags[entity]) & (1ULL << tag));     \
  } while (0)

typedef uint32_t eg_entity_t;

typedef struct eg_comp_pool_t {
  uint8_t *data;
} eg_comp_pool_t;

typedef struct eg_entity_manager_t {
  eg_entity_t entity_max; // Largest entity index + 1

  eg_comp_pool_t pools[EG_COMP_TYPE_MAX];
  bool comp_masks[EG_COMP_TYPE_MAX][EG_MAX_ENTITIES];
  bool existence[EG_MAX_ENTITIES];
  uint64_t tags[EG_MAX_ENTITIES];

  eg_entity_t to_remove[EG_MAX_ENTITIES];
  size_t to_remove_count;
} eg_entity_manager_t;

void eg_entity_manager_init(eg_entity_manager_t *entity_manager);

void eg_entity_manager_update(eg_entity_manager_t *entity_manager);

void eg_entity_manager_destroy(eg_entity_manager_t *entity_manager);

eg_entity_t eg_entity_add(eg_entity_manager_t *entity_manager);

void eg_entity_remove(eg_entity_manager_t *entity_manager, eg_entity_t entity);

bool eg_entity_exists(eg_entity_manager_t *entity_manager, eg_entity_t entity);

void *eg_comp_add(
    eg_entity_manager_t *entity_manager,
    eg_comp_type_t comp,
    eg_entity_t entity);

void eg_comp_remove(
    eg_entity_manager_t *entity_manager,
    eg_comp_type_t comp,
    eg_entity_t entity);
