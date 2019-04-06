#include "asset_manager.h"
#include "assets/environment_asset.h"
#include "assets/gltf_model_asset.h"
#include "assets/mesh_asset.h"
#include "assets/pbr_material_asset.h"
#include <assert.h>
#include <stdlib.h>

void eg_asset_manager_init(eg_asset_manager_t *asset_manager) {
  mtx_init(&asset_manager->allocator_mutex, mtx_plain);

  // Allocator with 16k blocks
  fstd_allocator_init(&asset_manager->allocator, 2 << 13);

  eg_register_asset(
      EG_ENVIRONMENT_ASSET_TYPE,
      (eg_asset_destructor_t)eg_environment_asset_destroy);
  eg_register_asset(
      EG_MESH_ASSET_TYPE, (eg_asset_destructor_t)eg_mesh_asset_destroy);
  eg_register_asset(
      EG_PBR_MATERIAL_ASSET_TYPE,
      (eg_asset_destructor_t)eg_pbr_material_asset_destroy);
  eg_register_asset(
      EG_GLTF_MODEL_ASSET_TYPE,
      (eg_asset_destructor_t)eg_gltf_model_asset_destroy);

  asset_manager->asset_count = 0;
  asset_manager->assets =
      (eg_asset_t **)calloc(EG_MAX_ASSETS, sizeof(eg_asset_t *));
}

eg_asset_t *
eg_asset_manager_alloc(eg_asset_manager_t *asset_manager, size_t size) {
  mtx_lock(&asset_manager->allocator_mutex);

  assert(asset_manager->asset_count < EG_MAX_ASSETS);

  eg_asset_t *asset = fstd_alloc(&asset_manager->allocator, (uint32_t)size);
  asset_manager->assets[asset_manager->asset_count++] = asset;

  mtx_unlock(&asset_manager->allocator_mutex);

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

  fstd_allocator_destroy(&asset_manager->allocator);

  mtx_destroy(&asset_manager->allocator_mutex);
}
