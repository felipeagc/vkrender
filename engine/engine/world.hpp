#pragma once

#include "camera.hpp"
#include "components/component_types.hpp"
#include "environment.hpp"
#include <fstd/bitset.h>

#define EG_MAX_ENTITIES 128

#define EG_GET_COMP(world, entity, comp_type)                                  \
  ((comp_type *)&(world)->components[EG_COMP_TYPE(                             \
      comp_type)][entity * eg_component_sizes[EG_COMP_TYPE(comp_type)]]);

typedef uint32_t eg_entity_t;

typedef FSTD_BITSET(EG_MAX_ENTITIES) eg_entities_bitset_t;

typedef struct {
  uint8_t bytes[EG_MAX_ENTITIES / 8];
} eg_bitset_t;

typedef struct eg_world_t {
  eg_camera_t camera;
  eg_environment_t environment;

  uint8_t *components[EG_COMPONENT_TYPE_COUNT];
  eg_entities_bitset_t component_bitsets[EG_COMPONENT_TYPE_COUNT];
} eg_world_t;

void eg_world_init(
    eg_world_t *world, eg_environment_asset_t *environment_asset);

eg_entity_t eg_world_add_entity(eg_world_t *world);

void eg_world_remove_entity(eg_world_t *world, eg_entity_t entity);

void *eg_world_add_comp(
    eg_world_t *world, eg_entity_t entity, eg_component_type_t comp);

bool eg_world_has_comp(
    eg_world_t *world, eg_entity_t entity, eg_component_type_t comp);

bool eg_world_has_any_comp(eg_world_t *world, eg_entity_t entity);

void eg_world_remove_comp(
    eg_world_t *world, eg_entity_t entity, eg_component_type_t comp);

void eg_world_destroy(eg_world_t *world);
