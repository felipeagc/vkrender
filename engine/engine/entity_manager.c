#include "entity_manager.h"

#include "util.h"
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

void eg_entity_manager_destroy(eg_entity_manager_t *entity_manager) {
  for (uint32_t e = 0; e < entity_manager->entity_max; e++) {
    for (uint32_t c = 0; c < EG_COMP_TYPE_MAX; c++) {
      eg_comp_remove(entity_manager, e, (eg_comp_type_t)c);
    }
  }

  for (uint32_t c = 0; c < EG_COMP_TYPE_MAX; c++) {
    free(entity_manager->pools[c].data);
  }
}

eg_entity_t eg_entity_add(eg_entity_manager_t *entity_manager) {
  for (uint32_t e = 0; e < EG_MAX_ENTITIES; e++) {
    if (!eg_entity_exists(entity_manager, e)) {
      entity_manager->existence[e] = true;
      if (entity_manager->entity_max <= e) {
        entity_manager->entity_max = e + 1;
      }
      return e;
    }
  }

  return UINT32_MAX;
}

void eg_entity_remove(eg_entity_manager_t *entity_manager, eg_entity_t entity) {
  if (!eg_entity_exists(entity_manager, entity)) {
    return;
  }

  entity_manager->existence[entity] = false;

  if (entity_manager->entity_max == entity + 1) {
    uint32_t new_max = 0;
    // @TODO: this is pretty expensive
    for (eg_entity_t e = 0; e < entity_manager->entity_max; e++) {
      if (eg_entity_exists(entity_manager, e)) {
        new_max = e + 1;
      }
    }
    entity_manager->entity_max = new_max;
  }

  for (uint32_t c = 0; c < EG_COMP_TYPE_MAX; c++) {
    entity_manager->comp_masks[c][entity] = false;

    if (EG_HAS_COMP_ID(entity_manager, entity, c)) {
      EG_COMP_DESTRUCTORS[c](
          &entity_manager->pools[c].data[entity * EG_COMP_SIZES[c]]);
    }
  }
}

bool eg_entity_exists(eg_entity_manager_t *entity_manager, eg_entity_t entity) {
  if (entity >= EG_MAX_ENTITIES) {
    return false;
  }

  return entity_manager->existence[entity];
}

void *eg_comp_add(
    eg_entity_manager_t *entity_manager,
    eg_entity_t entity,
    eg_comp_type_t comp) {
  entity_manager->comp_masks[comp][entity] = true;

  void *comp_ptr =
      &entity_manager->pools[comp].data[entity * EG_COMP_SIZES[comp]];
  EG_COMP_INITIALIZERS[comp](comp_ptr);

  return comp_ptr;
}

void eg_comp_remove(
    eg_entity_manager_t *entity_manager,
    eg_entity_t entity,
    eg_comp_type_t comp) {
  if (EG_HAS_COMP_ID(entity_manager, entity, comp)) {
    EG_COMP_DESTRUCTORS[comp](EG_COMP_BY_ID(entity_manager, entity, comp));
  }

  entity_manager->comp_masks[comp][entity] = false;
}
