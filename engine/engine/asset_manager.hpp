#pragma once

#include <util/bump_allocator.h>

#define eg_asset_alloc(asset_manager, type)                                    \
  ((type *)eg_asset_manager_alloc(asset_manager, sizeof(type), alignof(type)))

typedef struct eg_asset_manager_t {
  ut_bump_allocator_t allocator;
} eg_asset_manager_t;

void eg_asset_manager_init(eg_asset_manager_t *asset_manager);

void *eg_asset_manager_alloc(
    eg_asset_manager_t *asset_manager, size_t size, size_t alignment);

void eg_asset_manager_destroy(eg_asset_manager_t *asset_manager);
