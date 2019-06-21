#pragma once

#include "assets/asset_types.h"
#include <fstd_alloc.h>
#include <fstd_map.h>
#include <tinycthread.h>

typedef struct eg_asset_manager_t {
  mtx_t mutex;
  fstd_allocator_t allocator;
  eg_asset_t **assets;

  uint32_t cap;   /* The amount of allocated slots in the `assets` vector */
  uint32_t count; /* The index of the last asset in the `assets` vector + 1 */
  eg_asset_uid_t last_uid; /* The last asset UID (incremented after every asset
                        creation) */
} eg_asset_manager_t;

void eg_asset_manager_init(eg_asset_manager_t *asset_manager);

void *eg_asset_manager_alloc(
    eg_asset_manager_t *asset_manager, eg_asset_type_t asset_type);

void *eg_asset_manager_create(
    eg_asset_manager_t *asset_manager,
    eg_asset_type_t asset_type,
    const char *name,
    void *options);

eg_asset_t *
eg_asset_manager_get(eg_asset_manager_t *asset_manager, uint32_t index);

void eg_asset_manager_free(
    eg_asset_manager_t *asset_manager, eg_asset_t *asset);

void eg_asset_manager_destroy(eg_asset_manager_t *asset_manager);
