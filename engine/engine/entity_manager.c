#include "entity_manager.h"

#include <stdlib.h>
#include <string.h>

void eg_entity_manager_init(eg_entity_manager_t *entity_manager) {
  memset(entity_manager, 0, sizeof(*entity_manager));

  // Initialize component pools
  for (uint32_t comp = 0; comp < EG_COMP_TYPE_MAX; comp++) {
    entity_manager->pools[comp].data =
        calloc(EG_MAX_ENTITIES, EG_COMP_SIZES[comp]);
  }
}

void eg_entity_manager_update(eg_entity_manager_t *entity_manager) {
  for (uint32_t i = 0; i < entity_manager->to_remove_count; i++) {
    eg_entity_t entity = entity_manager->to_remove[i];

    if (eg_entity_exists(entity_manager, entity)) {
      fstd_bitset_set(&entity_manager->existence, entity, false);

      if (entity_manager->entity_max == entity + 1) {
        uint32_t new_max = 0;
        // @TODO: this is pretty expensive
        for (eg_entity_t e = 0; e < entity_manager->entity_max; e++) {
          if (eg_entity_exists(entity_manager, e) && e != entity) {
            new_max = e + 1;
          }
        }
        entity_manager->entity_max = new_max;
      }

      for (uint32_t c = 0; c < EG_COMP_TYPE_MAX; c++) {
        if (EG_HAS_COMP_ID(entity_manager, entity, c)) {
          EG_COMP_DESTRUCTORS[c](
              &entity_manager->pools[c].data[entity * EG_COMP_SIZES[c]]);
          fstd_bitset_set(&entity_manager->comp_masks[c], entity, false);
        }
      }
    }
  }

  entity_manager->to_remove_count = 0;
}

void eg_entity_manager_destroy(eg_entity_manager_t *entity_manager) {
  for (uint32_t e = 0; e < entity_manager->entity_max; e++) {
    for (uint32_t c = 0; c < EG_COMP_TYPE_MAX; c++) {
      eg_comp_remove(entity_manager, (eg_comp_type_t)c, e);
    }
  }

  for (uint32_t c = 0; c < EG_COMP_TYPE_MAX; c++) {
    free(entity_manager->pools[c].data);
  }
}

eg_entity_t eg_entity_add(eg_entity_manager_t *entity_manager) {
  for (uint32_t e = 0; e < EG_MAX_ENTITIES; e++) {
    if (!eg_entity_exists(entity_manager, e)) {
      fstd_bitset_set(&entity_manager->existence, e, true);
      if (entity_manager->entity_max <= e) {
        entity_manager->entity_max = e + 1;
      }
      return e;
    }
  }

  return UINT32_MAX;
}

void eg_entity_remove(eg_entity_manager_t *entity_manager, eg_entity_t entity) {
  if (eg_entity_exists(entity_manager, entity)) {
    entity_manager->to_remove[entity_manager->to_remove_count++] = entity;
  }
}

bool eg_entity_exists(eg_entity_manager_t *entity_manager, eg_entity_t entity) {
  if (entity >= EG_MAX_ENTITIES) {
    return false;
  }
  return fstd_bitset_at(&entity_manager->existence, entity);
}

void *eg_comp_add(
    eg_entity_manager_t *entity_manager,
    eg_comp_type_t comp,
    eg_entity_t entity) {
  fstd_bitset_set(&entity_manager->comp_masks[comp], entity, true);

  void *comp_ptr =
      &entity_manager->pools[comp].data[entity * EG_COMP_SIZES[comp]];
  EG_COMP_INITIALIZERS[comp](comp_ptr);

  return comp_ptr;
}

void eg_comp_remove(
    eg_entity_manager_t *entity_manager,
    eg_comp_type_t comp,
    eg_entity_t entity) {
  if (!EG_HAS_COMP_ID(entity_manager, entity, comp)) {
    return;
  }

  const size_t comp_size = EG_COMP_SIZES[comp];

  bool zeroed = true;
  for (uint32_t i = 0; i < comp_size; i++) {
    if (entity_manager->pools[comp].data[entity * comp_size + i] != 0) {
      zeroed = false;
      break;
    }
  }

  if (!zeroed) {
    EG_COMP_DESTRUCTORS[comp](
        &entity_manager->pools[comp].data[entity * comp_size]);
  }

  fstd_bitset_set(&entity_manager->comp_masks[comp], entity, false);
}
