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

static bool bitset_at(eg_bitset_t *bitset, uint32_t pos) {
  uint32_t index = pos / 8;
  uint32_t bit = pos % 8;

  return bitset->bytes[index] & (1 << (bit));
}

static void bitset_set(eg_bitset_t *bitset, uint32_t pos, bool val) {
  uint32_t index = pos / 8;
  uint32_t bit = pos % 8;
  bitset->bytes[index] = (val ? 1 : 0) << bit;
}

// Sets the bitset to zero
static void bitset_reset(eg_bitset_t *bitset) {
  for (uint32_t i = 0; i < EG_MAX_ENTITIES / 8; i++) {
    bitset->bytes[i] = 0;
  }
}

void eg_world_init(
    eg_world_t *world, eg_environment_asset_t *environment_asset) {
  eg_camera_init(&world->camera);
  eg_environment_init(&world->environment, environment_asset);

  // Initialize entities struct members
  EG_INIT_COMPS(world, eg_mesh_t);

  for (uint32_t i = 0; i < EG_COMP_COUNT; i++) {
    bitset_reset(&world->entities.components[i].bitset);
  }
}

eg_entity_t eg_world_add_entity(eg_world_t *world) {
  for (uint32_t e = 0; e < EG_MAX_ENTITIES; e++) {
    bool empty = true;
    for (uint32_t c = 0; c < EG_COMP_COUNT; c++) {
      if (bitset_at(&world->entities.components[c].bitset, e)) {
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
      bitset_set(&world->entities.components[c].bitset, entity, true);
    }
  }
}

void eg_world_add_comp(
    eg_world_t *world, eg_entity_t entity, eg_component_index_t comp) {
  bitset_set(&world->entities.components[comp].bitset, entity, true);
}

bool eg_world_has_comp(
    eg_world_t *world, eg_entity_t entity, eg_component_index_t comp) {
  return bitset_at(&world->entities.components[comp].bitset, entity);
}

void eg_world_remove_comp(
    eg_world_t *world, eg_entity_t entity, eg_component_index_t comp) {
  bitset_set(&world->entities.components[comp].bitset, entity, false);
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
