#include "asset_manager.hpp"

void eg_asset_manager_init(eg_asset_manager_t *asset_manager) {
  // Allocator with 16k blocks
  ut_bump_allocator_init(&asset_manager->allocator, 2 << 13);
}

void *eg_asset_manager_alloc(
    eg_asset_manager_t *asset_manager, size_t size, size_t alignment) {
  return ut_bump_allocator_alloc(&asset_manager->allocator, size, alignment);
}

void eg_asset_manager_destroy(eg_asset_manager_t *asset_manager) {
  ut_bump_allocator_destroy(&asset_manager->allocator);
}
