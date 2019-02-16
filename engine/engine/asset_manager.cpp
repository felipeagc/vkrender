#include "asset_manager.hpp"
#include "asset.hpp"
#include <assert.h>
#include <stdlib.h>

void eg_asset_manager_init(eg_asset_manager_t *asset_manager) {
  // Allocator with 16k blocks
  ut_bump_allocator_init(&asset_manager->allocator, 2 << 13);

  asset_manager->asset_count = 0;
  asset_manager->assets =
      (eg_asset_t **)malloc(sizeof(eg_asset_t *) * EG_MAX_ASSETS);

  for (uint32_t i = 0; i < EG_MAX_ASSETS; i++) {
    asset_manager->assets[i] = NULL;
  }
}

eg_asset_t *eg_asset_manager_alloc(
    eg_asset_manager_t *asset_manager, size_t size, size_t alignment) {
  assert(asset_manager->asset_count < EG_MAX_ASSETS);

  eg_asset_t *asset = (eg_asset_t *)ut_bump_allocator_alloc(
      &asset_manager->allocator, size, alignment);

  asset_manager->assets[asset_manager->asset_count++] = asset;

  return asset;
}

void eg_asset_manager_destroy(eg_asset_manager_t *asset_manager) {
  for (uint32_t i = 0; i < asset_manager->asset_count; i++) {
    eg_asset_t *asset = asset_manager->assets[i];

    if (asset == NULL)
      continue;
    if (asset->destructor == NULL)
      continue;

    eg_asset_destructor_t destructor = asset->destructor;

    destructor(asset);
  }

  free(asset_manager->assets);

  ut_bump_allocator_destroy(&asset_manager->allocator);
}
