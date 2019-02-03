#pragma once

#include <stddef.h>
#include <stdint.h>

#define ut_bump_alloc(allocator, type)                                            \
  ((type *)ut_bump_allocator_alloc(allocator, sizeof(type), alignof(type)))

struct ut_bump_block_t {
  uint8_t *storage;
  size_t filled;
  struct ut_bump_block_t *next_block;
};

struct ut_bump_allocator_t {
  ut_bump_block_t base_block;
  ut_bump_block_t *last_block;
  size_t block_size;
};

void ut_bump_allocator_init(ut_bump_allocator_t *allocator, size_t block_size);

void ut_bump_allocator_destroy(ut_bump_allocator_t *allocator);

// @TODO: make this function thread safe
void *ut_bump_allocator_alloc(
    ut_bump_allocator_t *allocator, size_t size, size_t alignment);
