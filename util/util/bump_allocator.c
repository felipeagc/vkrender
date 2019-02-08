#include "bump_allocator.h"
#include <stdlib.h>

static inline void bump_block_init(ut_bump_block_t *block, size_t block_size) {
  block->storage = (uint8_t *)malloc(block_size);
  block->filled = 0;
  block->next_block = NULL;
}

static inline void bump_block_destroy(ut_bump_block_t *block) {
  if (block == NULL) {
    return;
  }

  bump_block_destroy(block->next_block);
  if (block->next_block != NULL) {
    free(block->next_block);
  }

  free(block->storage);
}

void ut_bump_allocator_init(ut_bump_allocator_t *allocator, size_t block_size) {
  allocator->block_size = block_size;

  bump_block_init(&allocator->base_block, block_size);

  allocator->last_block = &allocator->base_block;

  ut_mutex_init(&allocator->mutex);
}

void ut_bump_allocator_destroy(ut_bump_allocator_t *allocator) {
  ut_mutex_lock(&allocator->mutex);
  bump_block_destroy(&allocator->base_block);
  ut_mutex_unlock(&allocator->mutex);
  ut_mutex_destroy(&allocator->mutex);
}

void *ut_bump_allocator_alloc(
    ut_bump_allocator_t *allocator, size_t size, size_t alignment) {
  ut_mutex_lock(&allocator->mutex);

  // For ensuring aligned memory allocations
  size_t padding = 0;
  if (allocator->last_block->filled % alignment != 0) {
    padding = alignment - (allocator->last_block->filled % alignment);
  }

  if ((size + padding) > allocator->block_size) {
    ut_mutex_unlock(&allocator->mutex);
    return NULL;
  }

  if ((allocator->last_block->filled + size + padding) >
      allocator->block_size) {
    ut_bump_block_t *new_block =
        (ut_bump_block_t *)malloc(sizeof(ut_bump_block_t));
    bump_block_init(new_block, allocator->block_size);

    allocator->last_block->next_block = new_block;
    allocator->last_block = new_block;
  }

  allocator->last_block->filled += padding;

  void *ptr =
      (void *)&allocator->last_block->storage[allocator->last_block->filled];

  allocator->last_block->filled += size;

  ut_mutex_unlock(&allocator->mutex);

  return ptr;
}
