#include "world.h"
#include <string.h>

#define EG__HAS_COMP(world, entity, comp_type)                                 \
  fstd_bitset_at(&world->comp_masks[comp_type], entity)

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

eg_entity_t eg_world_add(eg_world_t *world) {
  for (uint32_t e = 0; e < EG_MAX_ENTITIES; e++) {
    if (!eg_world_exists(world, e)) {
      fstd_bitset_set(&world->existence, e, true);
      world->entity_max = e + 1;
      return e;
    }
  }

  return UINT32_MAX;
}

void eg_world_remove(eg_world_t *world, eg_entity_t entity) {
  if (eg_world_exists(world, entity)) {
    fstd_bitset_set(&world->existence, entity, false);
    for (uint32_t e = entity - 1; e >= 0; e--) {
      if (eg_world_exists(world, e)) {
        world->entity_max = e + 1;
        break;
      }
    }

    for (uint32_t c = 0; c < EG_COMP_TYPE_MAX; c++) {
      EG_COMP_DESTRUCTORS[c](&world->pools[c].data[entity * EG_COMP_SIZES[c]]);
      fstd_bitset_set(&world->comp_masks[c], entity, false);
    }
  }
}

bool eg_world_exists(eg_world_t *world, eg_entity_t entity) {
  return fstd_bitset_at(&world->existence, entity);
}

void *
eg_world_add_comp(eg_world_t *world, eg_comp_type_t comp, eg_entity_t entity) {
  fstd_bitset_set(&world->comp_masks[comp], entity, true);
  memset(
      &world->pools[comp].data[entity * EG_COMP_SIZES[comp]],
      0,
      EG_COMP_SIZES[comp]);

  return &world->pools[comp].data[entity * EG_COMP_SIZES[comp]];
}

void eg_world_remove_comp(
    eg_world_t *world, eg_comp_type_t comp, eg_entity_t entity) {
  if (!EG__HAS_COMP(world, entity, comp)) {
    return;
  }

  bool zeroed = true;
  for (uint32_t i = 0; i < EG_COMP_SIZES[comp]; i++) {
    if (world->pools[comp].data[entity * EG_COMP_SIZES[comp] + i] != 0) {
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
