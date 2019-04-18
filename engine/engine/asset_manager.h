#pragma once

#include "assets/asset_types.h"
#include <fstd_alloc.h>
#include <fstd_map.h>
#include <tinycthread.h>

#define EG_MAX_ASSETS 521

#define eg_asset_alloc(asset_manager, name, type)                              \
  ((type *)eg_asset_manager_alloc(                                             \
      asset_manager, name, EG_ASSET_TYPE(type), sizeof(type)))

typedef struct eg_asset_manager_t {
  fstd_allocator_t allocator;
  mtx_t allocator_mutex;
  fstd_map_t map;
} eg_asset_manager_t;

void eg_asset_manager_init(eg_asset_manager_t *asset_manager);

eg_asset_t *
eg_asset_manager_get(eg_asset_manager_t *asset_manager, const char *name);

eg_asset_t *eg_asset_manager_get_by_index(
    eg_asset_manager_t *asset_manager, uint32_t index);

eg_asset_t *eg_asset_manager_alloc(
    eg_asset_manager_t *asset_manager,
    char *name,
    eg_asset_type_t type,
    size_t size);

void eg_asset_manager_free(
    eg_asset_manager_t *asset_manager, eg_asset_t *asset);

void eg_asset_manager_destroy(eg_asset_manager_t *asset_manager);
