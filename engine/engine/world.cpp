#include "world.hpp"
#include <string.h>

static inline void eg_init_comps(eg_world_t *world, eg_component_type_t comp) {
  world->entities.components[comp].array =
      (uint8_t *)calloc(EG_MAX_ENTITIES, eg_component_sizes[comp]);
}

void eg_world_init(
    eg_world_t *world, eg_environment_asset_t *environment_asset) {
  eg_camera_init(&world->camera);
  eg_environment_init(&world->environment, environment_asset);

  // Register component types
  EG_REGISTER_COMP(eg_mesh_component_t);

  // Initialize component types
  eg_init_comps(world, EG_MESH_COMPONENT_TYPE);

  for (uint32_t i = 0; i < EG_COMPONENT_TYPE_COUNT; i++) {
    ut_bitset_reset(
        (uint8_t *)&world->entities.components[i].bitset, EG_MAX_ENTITIES);
  }
}

eg_entity_t eg_world_add_entity(eg_world_t *world) {
  for (uint32_t e = 0; e < EG_MAX_ENTITIES; e++) {
    bool empty = true;
    for (uint32_t c = 0; c < EG_COMPONENT_TYPE_COUNT; c++) {
      if (ut_bitset_at((uint8_t *)&world->entities.components[c].bitset, e)) {
        empty = false;
        break;
      }
    }

    if (empty) {
      return e;
    }
  }

  return UINT32_MAX;
}

void eg_world_remove_entity(eg_world_t *world, eg_entity_t entity) {
  for (uint32_t e = 0; e < EG_MAX_ENTITIES; e++) {
    for (uint32_t c = 0; c < EG_COMPONENT_TYPE_COUNT; c++) {
      eg_component_destructors[c](
          &world->entities.components[c].array[entity * eg_component_sizes[c]]);
      ut_bitset_set(
          (uint8_t *)&world->entities.components[c].bitset, entity, true);
    }
  }
}

void *eg_world_add_comp(
    eg_world_t *world, eg_entity_t entity, eg_component_type_t comp) {
  ut_bitset_set(
      (uint8_t *)&world->entities.components[comp].bitset, entity, true);
  memset(
      &world->entities.components[comp]
           .array[entity * eg_component_sizes[comp]],
      0,
      eg_component_sizes[comp]);

  return &world->entities.components[comp]
              .array[entity * eg_component_sizes[comp]];
}

bool eg_world_has_comp(
    eg_world_t *world, eg_entity_t entity, eg_component_type_t comp) {
  return ut_bitset_at(
      (uint8_t *)&world->entities.components[comp].bitset, entity);
}

void eg_world_remove_comp(
    eg_world_t *world, eg_entity_t entity, eg_component_type_t comp) {
  if (!eg_world_has_comp(world, entity, comp)) {
    return;
  }

  bool zeroed = true;
  for (uint32_t i = 0; i < eg_component_sizes[comp]; i++) {
    if (world->entities.components[comp]
            .array[entity * eg_component_sizes[comp] + i] != 0) {
      zeroed = false;
      break;
    }
  }

  if (!zeroed) {
    eg_component_destructors[comp](
        &world->entities.components[comp]
             .array[entity * eg_component_sizes[comp]]);
  }

  ut_bitset_set(
      (uint8_t *)&world->entities.components[comp].bitset, entity, false);
}

void eg_world_destroy(eg_world_t *world) {
  for (uint32_t e = 0; e < EG_MAX_ENTITIES; e++) {
    for (uint32_t c = 0; c < EG_COMPONENT_TYPE_COUNT; c++) {
      eg_world_remove_comp(world, e, (eg_component_type_t)c);
    }
  }

  for (uint32_t c = 0; c < EG_COMPONENT_TYPE_COUNT; c++) {
    free(world->entities.components[c].array);
  }

  eg_environment_destroy(&world->environment);
  eg_camera_destroy(&world->camera);
}
