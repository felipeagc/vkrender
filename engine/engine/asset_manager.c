#include "asset_manager.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MAX(a, b) ((a > b) ? a : b)

void eg_asset_manager_init(eg_asset_manager_t *asset_manager) {
  mtx_init(&asset_manager->mutex, mtx_plain);

  // Allocator with 16k blocks
  fstd_allocator_init(&asset_manager->allocator, 16384);

  asset_manager->cap      = 128;
  asset_manager->count    = 0;
  asset_manager->next_uid = 0;
  asset_manager->assets   = realloc(
      asset_manager->assets,
      asset_manager->cap * sizeof(*asset_manager->assets));
  memset(
      asset_manager->assets,
      0,
      asset_manager->cap * sizeof(*asset_manager->assets));
}

void *eg_asset_manager_alloc(
    eg_asset_manager_t *asset_manager, eg_asset_type_t asset_type) {
  void *asset = eg_asset_manager_alloc_uid(
      asset_manager, asset_type, asset_manager->next_uid);

  return asset;
}

void *eg_asset_manager_alloc_uid(
    eg_asset_manager_t *asset_manager,
    eg_asset_type_t asset_type,
    eg_asset_uid_t uid) {
  mtx_lock(&asset_manager->mutex);

  asset_manager->next_uid = MAX(uid + 1, asset_manager->next_uid);

  eg_asset_t *asset = fstd_alloc(
      &asset_manager->allocator, (uint32_t)EG_ASSET_SIZES[asset_type]);
  memset(asset, 0, sizeof(*asset));

  uint32_t index = UINT32_MAX;

  // Look for the first empty asset slot
  for (uint32_t i = 0; i < asset_manager->cap; i++) {
    if (asset_manager->assets[i] == NULL) {
      index = i;
      break;
    }
  }

  // If no available slot was found
  if (index == UINT32_MAX) {
    // Double the amount of slots
    asset_manager->cap *= 2;
    asset_manager->assets = realloc(
        asset_manager->assets,
        asset_manager->cap * sizeof(*asset_manager->assets));

    // Set the new ones to NULL
    memset(
        &asset_manager->assets[asset_manager->cap / 2],
        0,
        (asset_manager->cap / 2) * sizeof(*asset_manager->assets));

    // Set the index to the first newly created slot
    index = asset_manager->cap / 2;
  }

  asset_manager->assets[index] = asset;

  asset_manager->count =
      (index >= asset_manager->count) ? index + 1 : asset_manager->count;

  asset->type  = asset_type;
  asset->index = index;
  asset->uid   = uid;

  mtx_unlock(&asset_manager->mutex);

  return asset;
}

void *eg_asset_manager_get(eg_asset_manager_t *asset_manager, uint32_t index) {
  return asset_manager->assets[index];
}

void *eg_asset_manager_get_by_uid(
    eg_asset_manager_t *asset_manager, eg_asset_uid_t uid) {
  for (uint32_t i = 0; i < asset_manager->count; i++) {
    eg_asset_t *asset = eg_asset_manager_get(asset_manager, i);
    if (asset != NULL) {
      if (asset->uid == uid) return asset;
    }
  }

  return NULL;
}

void eg_asset_manager_free(
    eg_asset_manager_t *asset_manager, eg_asset_t *asset) {
  if (asset == NULL) return;

  if (asset->name != NULL) free(asset->name);

  EG_ASSET_DESTRUCTORS[asset->type](asset);

  mtx_lock(&asset_manager->mutex);
  fstd_free(&asset_manager->allocator, asset);
  asset_manager->assets[asset->index] = NULL;

  // Shorten the count if possible
  // TODO: this loop is probably slow
  uint32_t count = 0;
  for (uint32_t i = 0; i < asset_manager->count; i++) {
    if (asset_manager->assets[i] != NULL) {
      count = i + 1;
    }
  }

  asset_manager->count = count;

  mtx_unlock(&asset_manager->mutex);
}

void eg_asset_manager_destroy(eg_asset_manager_t *asset_manager) {
  const uint32_t count = asset_manager->count;

  for (uint32_t i = 0; i < count; i++) {
    eg_asset_t *asset = eg_asset_manager_get(asset_manager, i);

    if (asset == NULL) continue;

    eg_asset_manager_free(asset_manager, asset);
  }

  assert(asset_manager->count == 0);

  mtx_lock(&asset_manager->mutex);
  fstd_allocator_destroy(&asset_manager->allocator);
  free(asset_manager->assets);
  mtx_unlock(&asset_manager->mutex);

  mtx_destroy(&asset_manager->mutex);
}
