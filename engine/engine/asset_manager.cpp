#include "asset_manager.hpp"
#include "assets/asset_types.hpp"
#include "assets/environment_asset.hpp"
#include "assets/mesh_asset.hpp"
#include "assets/pbr_material_asset.hpp"
#include <assert.h>
#include <stdlib.h>

void eg_asset_manager_init(eg_asset_manager_t *asset_manager) {
  // Allocator with 16k blocks
  ut_bump_allocator_init(&asset_manager->allocator, 2 << 13);

  eg_register_asset(
      EG_ENVIRONMENT_ASSET_TYPE,
      (eg_asset_destructor_t)eg_environment_asset_destroy);
  eg_register_asset(
      EG_MESH_ASSET_TYPE, (eg_asset_destructor_t)eg_mesh_asset_destroy);
  eg_register_asset(
      EG_PBR_MATERIAL_ASSET_TYPE,
      (eg_asset_destructor_t)eg_pbr_material_asset_destroy);

  asset_manager->asset_count = 0;
  asset_manager->assets =
      (eg_asset_t **)calloc(EG_MAX_ASSETS, sizeof(eg_asset_t *));
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

    eg_asset_destructors[asset->type](asset);
  }

  free(asset_manager->assets);

  ut_bump_allocator_destroy(&asset_manager->allocator);
}
