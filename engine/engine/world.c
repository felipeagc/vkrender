#include "world.h"
#include <string.h>

#define EG__HAS_COMP(world, entity, comp_type)                                 \
  fstd_bitset_at(&world->comp_masks[comp_type], entity)

static eg_entity_t to_remove[EG_MAX_ENTITIES] = {0};
static size_t to_remove_count = 0;

void eg_world_init(
    eg_world_t *world, eg_environment_asset_t *environment_asset) {
  eg_camera_init(&world->camera);
  eg_environment_init(&world->environment, environment_asset);

  world->entity_max = 0;

  // Initialize component pools
  for (uint32_t comp = 0; comp < EG_COMP_TYPE_MAX; comp++) {
    world->pools[comp].data = calloc(EG_MAX_ENTITIES, EG_COMP_SIZES[comp]);
  }

  for (uint32_t i = 0; i < EG_COMP_TYPE_MAX; i++) {
    fstd_bitset_reset(&world->comp_masks[i], EG_MAX_ENTITIES);
  }
  fstd_bitset_reset(&world->existence, EG_MAX_ENTITIES);

  for (uint32_t i = 0; i < EG_MAX_ENTITIES; i++) {
    world->tags[i] = 0;
  }
}

void eg_world_update(eg_world_t *world) {
  for (uint32_t i = 0; i < to_remove_count; i++) {
    eg_entity_t entity = to_remove[i];

    if (eg_world_exists(world, entity)) {
      fstd_bitset_set(&world->existence, entity, false);

      if (world->entity_max == entity + 1) {
        uint32_t new_max = 0;
        // @TODO: this is pretty expensive
        for (eg_entity_t e = 0; e < world->entity_max; e++) {
          if (eg_world_exists(world, e) && e != entity) {
            new_max = e + 1;
          }
        }
        world->entity_max = new_max;
      }

      for (uint32_t c = 0; c < EG_COMP_TYPE_MAX; c++) {
        if (EG__HAS_COMP(world, entity, c)) {
          EG_COMP_DESTRUCTORS[c](
              &world->pools[c].data[entity * EG_COMP_SIZES[c]]);
          fstd_bitset_set(&world->comp_masks[c], entity, false);
        }
      }
    }
  }

  to_remove_count = 0;
}

eg_entity_t eg_world_add(eg_world_t *world) {
  for (uint32_t e = 0; e < EG_MAX_ENTITIES; e++) {
    if (!eg_world_exists(world, e)) {
      fstd_bitset_set(&world->existence, e, true);
      if (world->entity_max <= e) {
        world->entity_max = e + 1;
      }
      return e;
    }
  }

  return UINT32_MAX;
}

void eg_world_remove(eg_world_t *world, eg_entity_t entity) {
  if (eg_world_exists(world, entity)) {
    to_remove[to_remove_count++] = entity;
  }
}

bool eg_world_exists(eg_world_t *world, eg_entity_t entity) {
  if (entity >= EG_MAX_ENTITIES) {
    return false;
  }
  return fstd_bitset_at(&world->existence, entity);
}

void *
eg_world_add_comp(eg_world_t *world, eg_comp_type_t comp, eg_entity_t entity) {
  fstd_bitset_set(&world->comp_masks[comp], entity, true);

  void *comp_ptr = &world->pools[comp].data[entity * EG_COMP_SIZES[comp]];
  EG_COMP_INITIALIZERS[comp](comp_ptr);

  return comp_ptr;
}

void eg_world_remove_comp(
    eg_world_t *world, eg_comp_type_t comp, eg_entity_t entity) {
  if (!EG__HAS_COMP(world, entity, comp)) {
    return;
  }

  const uint32_t comp_size = EG_COMP_SIZES[comp];

  bool zeroed = true;
  for (uint32_t i = 0; i < comp_size; i++) {
    if (world->pools[comp].data[entity * comp_size + i] != 0) {
      zeroed = false;
      break;
    }
  }

  if (!zeroed) {
    EG_COMP_DESTRUCTORS[comp](
        &world->pools[comp].data[entity * EG_COMP_SIZES[comp]]);
  }

  fstd_bitset_set(&world->comp_masks[comp], entity, false);
}

void eg_world_destroy(eg_world_t *world) {
  for (uint32_t e = 0; e < world->entity_max; e++) {
    for (uint32_t c = 0; c < EG_COMP_TYPE_MAX; c++) {
      eg_world_remove_comp(world, (eg_comp_type_t)c, e);
    }
  }

  for (uint32_t c = 0; c < EG_COMP_TYPE_MAX; c++) {
    free(world->pools[c].data);
  }

  eg_environment_destroy(&world->environment);
  eg_camera_destroy(&world->camera);
}
