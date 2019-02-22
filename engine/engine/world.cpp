#include "world.hpp"

#define EG_INIT_COMPS(world, comp_type)                                        \
  (world)->entities.components[EG_COMP_INDEX(comp_type)].array =               \
      (comp_type *)malloc(sizeof(comp_type) * EG_MAX_ENTITIES);

#define EG_DESTROY_COMPS(world, comp_type)                                     \
  for (uint32_t e = 0; e < EG_MAX_ENTITIES; e++) {                             \
    if (eg_world_has_comp(world, e, EG_COMP_INDEX(comp_type))) {               \
      EG_COMP_DESTROY(comp_type)(EG_GET_COMP(world, comp_type, e));            \
    }                                                                          \
  }

void eg_world_init(
    eg_world_t *world, eg_environment_asset_t *environment_asset) {
  eg_camera_init(&world->camera);
  eg_environment_init(&world->environment, environment_asset);

  // Initialize entities struct members
  EG_INIT_COMPS(world, eg_mesh_t);

  for (uint32_t i = 0; i < EG_COMP_COUNT; i++) {
    ut_bitset_reset(
        (uint8_t *)&world->entities.components[i].bitset, EG_MAX_ENTITIES);
  }
}

eg_entity_t eg_world_add_entity(eg_world_t *world) {
  for (uint32_t e = 0; e < EG_MAX_ENTITIES; e++) {
    bool empty = true;
    for (uint32_t c = 0; c < EG_COMP_COUNT; c++) {
      if (ut_bitset_at(
              (uint8_t *)&world->entities.components[c].bitset, e)) {
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
    for (uint32_t c = 0; c < EG_COMP_COUNT; c++) {
      ut_bitset_set(
          (uint8_t *)&world->entities.components[c].bitset, entity, true);
    }
  }
}

void eg_world_add_comp(
    eg_world_t *world, eg_entity_t entity, eg_component_index_t comp) {
  ut_bitset_set(
      (uint8_t *)&world->entities.components[comp].bitset, entity, true);
}

bool eg_world_has_comp(
    eg_world_t *world, eg_entity_t entity, eg_component_index_t comp) {
  return ut_bitset_at(
      (uint8_t *)&world->entities.components[comp].bitset, entity);
}

void eg_world_remove_comp(
    eg_world_t *world, eg_entity_t entity, eg_component_index_t comp) {
  ut_bitset_set(
      (uint8_t *)&world->entities.components[comp].bitset, entity, false);
}

void eg_world_destroy(eg_world_t *world) {
  // Destroy components
  EG_DESTROY_COMPS(world, eg_mesh_t);

  for (uint32_t c = 0; c < EG_COMP_COUNT; c++) {
    free(world->entities.components[c].array);
  }

  eg_environment_destroy(&world->environment);
  eg_camera_destroy(&world->camera);
}
