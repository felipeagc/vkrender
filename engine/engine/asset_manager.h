#pragma once

#include "assets/asset_types.h"
#include <fstd_alloc.h>
#include <fstd_map.h>
#include <tinycthread.h>

#define eg_asset_alloc(asset_manager, type)                                    \
  ((type *)eg_asset_manager_alloc(asset_manager, EG_ASSET_TYPE(type)))

typedef struct eg_asset_manager_t {
  mtx_t mutex;
  fstd_allocator_t allocator;
  eg_asset_t **assets;
  uint32_t cap;
  uint32_t max_index;
} eg_asset_manager_t;

void eg_asset_manager_init(eg_asset_manager_t *asset_manager);

eg_asset_t *eg_asset_manager_alloc(
    eg_asset_manager_t *asset_manager, eg_asset_type_t asset_type);

eg_asset_t *
eg_asset_manager_get(eg_asset_manager_t *asset_manager, uint32_t index);

void eg_asset_manager_free(
    eg_asset_manager_t *asset_manager, eg_asset_t *asset);

void eg_asset_manager_destroy(eg_asset_manager_t *asset_manager);
