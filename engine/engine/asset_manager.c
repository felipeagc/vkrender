#include "asset_manager.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/*
 *
 * Asset hash table
 *
 */

static inline uint64_t djb_hash(const unsigned char *str) {
  uint64_t hash = 5381;
  uint32_t c;

  while ((c = *str++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

  return hash;
}

typedef enum eg_asset_table_value_state_t {
  ASSET_TABLE_VALUE_EMPTY = 0,
  ASSET_TABLE_VALUE_DELETED,
  ASSET_TABLE_VALUE_FILLED,
} eg_asset_table_value_state_t;

typedef struct eg_asset_table_bundle_t {
  eg_asset_table_value_state_t state;
  eg_asset_t *asset;
} eg_asset_table_bundle_t;

void eg_asset_table_init(eg_asset_table_t *table, uint32_t capacity) {
  table->capacity = capacity;
  table->filled = 0;
  table->data = calloc(table->capacity, sizeof(*table->data));
}

bool eg_asset_table_set(eg_asset_table_t *table, char *key, eg_asset_t *value) {
  uint64_t hash = djb_hash((unsigned char *)key);
  uint32_t index = hash % table->capacity;
  uint32_t index_start = index;

  uint32_t first_deleted = index;

  bool found_existing = false;
  bool found_deleted = false;
  while (table->data[index].state != ASSET_TABLE_VALUE_EMPTY) {
    // Collision!
    if (!found_deleted &&
        table->data[index].state == ASSET_TABLE_VALUE_DELETED) {
      first_deleted = index;
      found_deleted = true;
    }

    if (table->data[index].asset != NULL &&
        strcmp(table->data[index].asset->name, key) == 0) {
      found_existing = true;
      break;
    }

    index = (index + 1) % table->capacity;
    if (index == index_start) {
      break;
    }
  }

  if (!found_existing && table->filled == table->capacity) {
    return false;
  }

  if (!found_existing &&
      table->data[first_deleted].state == ASSET_TABLE_VALUE_DELETED) {
    index = first_deleted;
  }

  table->data[index].asset = value;

  if (table->data[index].state != ASSET_TABLE_VALUE_FILLED) {
    table->filled++;
    table->data[index].state = ASSET_TABLE_VALUE_FILLED;
  }

  return true;
}

bool eg_asset_table_remove(eg_asset_table_t *table, char *key) {
  uint64_t hash = djb_hash((unsigned char *)key);
  uint32_t index = hash % table->capacity;
  uint32_t index_start = index;

  while (table->data[index].asset == NULL ||
         strcmp(table->data[index].asset->name, key) != 0) {
    index = (index + 1) % table->capacity;
    if (index == index_start) {
      return false;
    }
  }

  table->data[index].state = ASSET_TABLE_VALUE_DELETED;
  table->data[index].asset = NULL;
  table->filled--;
  return true;
}

eg_asset_t *eg_asset_table_get(eg_asset_table_t *table, const char *key) {
  uint64_t hash = djb_hash((const unsigned char *)key);
  uint32_t index = hash % table->capacity;
  uint32_t index_start = index;

  while (table->data[index].state != ASSET_TABLE_VALUE_FILLED ||
         strcmp(table->data[index].asset->name, key) != 0) {
    index = (index + 1) % table->capacity;

    if (index == index_start) {
      return NULL;
    }
  }

  return table->data[index].asset;
}

void eg_asset_table_destroy(eg_asset_table_t *table) { free(table->data); }

/*
 *
 * Asset manager
 *
 */

void eg_asset_manager_init(eg_asset_manager_t *asset_manager) {
  mtx_init(&asset_manager->allocator_mutex, mtx_plain);

  // Allocator with 16k blocks
  fstd_allocator_init(&asset_manager->allocator, 2 << 13);

  eg_asset_table_init(&asset_manager->table, EG_MAX_ASSETS);
}

eg_asset_t *
eg_asset_manager_get(eg_asset_manager_t *asset_manager, const char *name) {
  return eg_asset_table_get(&asset_manager->table, name);
}

eg_asset_t *eg_asset_manager_get_by_index(
    eg_asset_manager_t *asset_manager, uint32_t index) {
  if (index >= EG_MAX_ASSETS) {
    return NULL;
  }

  return asset_manager->table.data[index].asset;
}

eg_asset_t *eg_asset_manager_alloc(
    eg_asset_manager_t *asset_manager,
    char *name,
    eg_asset_type_t type,
    size_t size) {
  mtx_lock(&asset_manager->allocator_mutex);

  eg_asset_t *asset = fstd_alloc(&asset_manager->allocator, (uint32_t)size);

  // TODO: allow replacing table entries
  assert(eg_asset_table_get(&asset_manager->table, name) == NULL);

  eg_asset_init(asset, type, name);
  assert(eg_asset_table_set(&asset_manager->table, name, asset));

  mtx_unlock(&asset_manager->allocator_mutex);

  return asset;
}

void eg_asset_manager_free(
    eg_asset_manager_t *asset_manager, eg_asset_t *asset) {
  if (asset == NULL) {
    return;
  }

  eg_asset_table_remove(&asset_manager->table, asset->name);

  eg_asset_destructors[asset->type](asset);
  eg_asset_destroy(asset);

  mtx_lock(&asset_manager->allocator_mutex);
  fstd_free(&asset_manager->allocator, asset);
  mtx_unlock(&asset_manager->allocator_mutex);
}

void eg_asset_manager_destroy(eg_asset_manager_t *asset_manager) {
  for (uint32_t i = 0; i < EG_MAX_ASSETS; i++) {
    eg_asset_t *asset = asset_manager->table.data[i].asset;

    if (asset == NULL)
      continue;

    eg_asset_destructors[asset->type](asset);
    eg_asset_destroy(asset);
  }

  eg_asset_table_destroy(&asset_manager->table);

  fstd_allocator_destroy(&asset_manager->allocator);

  mtx_destroy(&asset_manager->allocator_mutex);
}

