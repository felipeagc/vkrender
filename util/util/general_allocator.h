#ifndef UT_GENERAL_ALLOCATOR_H
#define UT_GENERAL_ALLOCATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "thread.h"
#include <stddef.h>
#include <stdint.h>

#define UT_GENERAL_ALLOC(allocator, type)                                      \
  (type *)ut_general_allocator_alloc(allocator, sizeof(type))

typedef struct ut_general_alloc_header_t {
  /* 0x00 */ uint32_t size;
  /* 0x04 */ uint32_t used;
  /* 0x0C */ void *addr;
  /* 0x10 */ struct ut_general_alloc_header_t *prev;
  /* 0x18 */ struct ut_general_alloc_header_t *next;
} ut_general_alloc_header_t;

typedef struct ut_general_alloc_block_t {
  uint8_t *storage;
  ut_general_alloc_header_t *first_header;
  struct ut_general_alloc_block_t *next;
  struct ut_general_alloc_block_t *prev;
} ut_general_block_t;

typedef struct ut_general_allocator_t {
  ut_general_block_t base_block;
  ut_general_block_t *last_block;
  size_t block_size;
  ut_mutex_t mutex;
} ut_general_allocator_t;

void ut_general_allocator_init(
    ut_general_allocator_t *allocator, size_t block_size);

void ut_general_allocator_destroy(ut_general_allocator_t *allocator);

void *
ut_general_allocator_alloc(ut_general_allocator_t *allocator, uint32_t size);

void ut_general_allocator_free(ut_general_allocator_t *allocator, void *ptr);

#ifdef __cplusplus
}
#endif

#endif
