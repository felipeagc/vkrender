#pragma once

#include <stddef.h>
#include <stdint.h>

#define bump_alloc(allocator, type)                                            \
  ((type *)bump_allocator_alloc(allocator, sizeof(type), alignof(type)))

struct bump_block_t {
  uint8_t *storage;
  size_t filled;
  struct bump_block_t *next_block;
};

struct bump_allocator_t {
  bump_block_t base_block;
  bump_block_t *last_block;
  size_t block_size;
};

void bump_block_init(bump_block_t *block, size_t block_size);

void bump_block_destroy(bump_block_t *block);

void bump_allocator_init(bump_allocator_t *allocator, size_t block_size);

void bump_allocator_destroy(bump_allocator_t *allocator);

void *bump_allocator_alloc(
    bump_allocator_t *allocator, size_t size, size_t alignment);
