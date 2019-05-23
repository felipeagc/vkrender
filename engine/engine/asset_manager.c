#include "asset_manager.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/*
 *
 * Asset manager
 *
 */

void eg_asset_manager_init(eg_asset_manager_t *asset_manager) {
  mtx_init(&asset_manager->mutex, mtx_plain);

  // Allocator with 16k blocks
  fstd_allocator_init(&asset_manager->allocator, 2 << 13);

  fstd_map_init(&asset_manager->map, EG_MAX_ASSETS, eg_asset_t *);
}

eg_asset_t *
eg_asset_manager_get(eg_asset_manager_t *asset_manager, const char *name) {
  return fstd_map_get(&asset_manager->map, name);
}

eg_asset_t *eg_asset_manager_get_by_index(
    eg_asset_manager_t *asset_manager, uint32_t index) {
  if (index >= EG_MAX_ASSETS) {
    return NULL;
  }

  eg_asset_t **asset = fstd_map_get_by_index(&asset_manager->map, index, NULL);

  if (asset == NULL) {
    return NULL;
  }

  return *asset;
}

eg_asset_t *eg_asset_manager_alloc(
    eg_asset_manager_t *asset_manager,
    const char *name,
    eg_asset_type_t type,
    size_t size) {
  mtx_lock(&asset_manager->mutex);

  eg_asset_t *asset = fstd_alloc(&asset_manager->allocator, (uint32_t)size);

  // TODO: allow replacing map entries
  assert(fstd_map_get(&asset_manager->map, name) == NULL);

  eg_asset_t **asset_entry = fstd_map_set(&asset_manager->map, name, &asset);
  asset->type = type;
  asset->name = fstd_map_get_key(&asset_manager->map, asset_entry);

  mtx_unlock(&asset_manager->mutex);

  return asset;
}

void eg_asset_manager_free(
    eg_asset_manager_t *asset_manager, eg_asset_t *asset) {
  if (asset == NULL) {
    return;
  }

  fstd_map_remove(&asset_manager->map, asset->name);

  eg_asset_destructors[asset->type](asset);

  mtx_lock(&asset_manager->mutex);
  fstd_free(&asset_manager->allocator, asset);
  mtx_unlock(&asset_manager->mutex);
}

void eg_asset_manager_destroy(eg_asset_manager_t *asset_manager) {
  for (uint32_t i = 0; i < EG_MAX_ASSETS; i++) {
    eg_asset_t *asset = eg_asset_manager_get_by_index(asset_manager, i);

    if (asset == NULL)
      continue;

    eg_asset_destructors[asset->type](asset);
  }

  fstd_map_destroy(&asset_manager->map);

  mtx_lock(&asset_manager->mutex);
  fstd_allocator_destroy(&asset_manager->allocator);
  mtx_unlock(&asset_manager->mutex);

  mtx_destroy(&asset_manager->mutex);
}

