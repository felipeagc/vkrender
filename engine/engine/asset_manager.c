#include "asset_manager.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

void eg_asset_manager_init(eg_asset_manager_t *asset_manager) {
  mtx_init(&asset_manager->mutex, mtx_plain);

  // Allocator with 16k blocks
  fstd_allocator_init(&asset_manager->allocator, 16384);

  asset_manager->cap    = 128;
  asset_manager->count  = 0;
  asset_manager->assets = realloc(
      asset_manager->assets,
      asset_manager->cap * sizeof(*asset_manager->assets));
  memset(
      asset_manager->assets,
      0,
      asset_manager->cap * sizeof(*asset_manager->assets));
}

void *eg_asset_manager_alloc(
    eg_asset_manager_t *asset_manager, eg_asset_type_t asset_type) {
  mtx_lock(&asset_manager->mutex);

  eg_asset_t *asset = fstd_alloc(
      &asset_manager->allocator, (uint32_t)EG_ASSET_SIZES[asset_type]);
  memset(asset, 0, sizeof(*asset));

  uint32_t index = UINT32_MAX;

  for (uint32_t i = 0; i < asset_manager->cap; i++) {
    if (asset_manager->assets[i] == NULL) {
      index = i;
      break;
    }
  }

  // If no available slot was found
  if (index == UINT32_MAX) {
    asset_manager->cap *= 2;

    asset_manager->assets = realloc(
        asset_manager->assets,
        asset_manager->cap * sizeof(*asset_manager->assets));
    memset(
        &asset_manager->assets[asset_manager->cap / 2],
        0,
        (asset_manager->cap / 2) * sizeof(*asset_manager->assets));

    index = asset_manager->cap / 2;
  }

  asset_manager->assets[index] = asset;

  asset_manager->count =
      (index >= asset_manager->count) ? index + 1 : asset_manager->count;

  asset->type  = asset_type;
  asset->index = index;

  mtx_unlock(&asset_manager->mutex);

  return asset;
}

void *eg_asset_manager_create(
    eg_asset_manager_t *asset_manager,
    eg_asset_type_t asset_type,
    const char *name,
    void *options) {
  eg_asset_t *asset = EG_ASSET_CONSTRUCTORS[asset_type](asset_manager, options);

  if (asset != NULL) eg_asset_set_name(asset, name);

  return asset;
}

eg_asset_t *
eg_asset_manager_get(eg_asset_manager_t *asset_manager, uint32_t index) {
  return asset_manager->assets[index];
}

void eg_asset_manager_free(
    eg_asset_manager_t *asset_manager, eg_asset_t *asset) {
  if (asset == NULL) return;

  uint32_t index = asset->index;

  if (asset_manager->count == index) {
    for (uint32_t i = index; i > 0; i--) {
      if (asset_manager->assets[i - 1] != NULL) {
        asset_manager->count = i - 1;
        break;
      }
    }
  }

  if (asset->name != NULL) free(asset->name);

  EG_ASSET_DESTRUCTORS[asset->type](asset);

  mtx_lock(&asset_manager->mutex);
  fstd_free(&asset_manager->allocator, asset);
  asset_manager->assets[index] = NULL;
  mtx_unlock(&asset_manager->mutex);
}

void eg_asset_manager_destroy(eg_asset_manager_t *asset_manager) {
  const uint32_t count = asset_manager->count;

  for (uint32_t i = 0; i < count; i++) {
    eg_asset_t *asset = eg_asset_manager_get(asset_manager, i);

    if (asset == NULL) continue;

    eg_asset_manager_free(asset_manager, asset);
  }

  mtx_lock(&asset_manager->mutex);
  fstd_allocator_destroy(&asset_manager->allocator);
  free(asset_manager->assets);
  mtx_unlock(&asset_manager->mutex);

  mtx_destroy(&asset_manager->mutex);
}
