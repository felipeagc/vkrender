#ifndef FSTD_ALLOC_H
#define FSTD_ALLOC_H

/*
 * NOTE: the library is not thread safe. You will have to handle that yourself.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

typedef struct fstd_alloc_header_t {
  /* 0x00 */ uint32_t size;
  /* 0x04 */ uint32_t used;
  /* 0x0C */ void *addr;
  /* 0x10 */ struct fstd_alloc_header_t *prev;
  /* 0x18 */ struct fstd_alloc_header_t *next;
} fstd_alloc_header_t;

typedef struct fstd_alloc_block_t {
  uint8_t *storage;
  fstd_alloc_header_t *first_header;
  struct fstd_alloc_block_t *next;
  struct fstd_alloc_block_t *prev;
} fstd_alloc_block_t;

typedef struct fstd_allocator_t {
  fstd_alloc_block_t base_block;
  fstd_alloc_block_t *last_block;
  size_t block_size;
} fstd_allocator_t;

void fstd_allocator_init(fstd_allocator_t *allocator, size_t block_size);

void fstd_allocator_destroy(fstd_allocator_t *allocator);

void *fstd_alloc(fstd_allocator_t *allocator, uint32_t size);

void fstd_free(fstd_allocator_t *allocator, void *ptr);

#ifdef FSTD_ALLOC_IMPLEMENTATION

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

static inline void alloc_header_init(
    fstd_alloc_header_t *header,
    size_t size,
    fstd_alloc_header_t *prev,
    fstd_alloc_header_t *next) {
  header->addr = ((uint8_t *)header) + sizeof(fstd_alloc_header_t);
  header->prev = prev;
  header->next = next;
  header->size = (uint32_t)size;
  header->used = false;
}

static inline void
alloc_header_merge_if_necessary(fstd_alloc_header_t *header) {
  assert(header != NULL);
  assert(!header->used);

  if (header->prev != NULL && !header->prev->used) {
    header->prev->next = header->next;
    header->prev->size += header->size + sizeof(fstd_alloc_header_t);
    header = header->prev;
  }

  if (header->next != NULL && !header->next->used) {
    header->size += header->next->size + sizeof(fstd_alloc_header_t);
    header->next = header->next->next;
  }
}

static inline void
general_block_init(fstd_alloc_block_t *block, size_t block_size) {
  assert(block_size > sizeof(fstd_alloc_header_t));
  block->storage = (uint8_t *)malloc(block_size);
  block->first_header = (fstd_alloc_header_t *)block->storage;
  alloc_header_init(
      block->first_header,
      block_size - sizeof(fstd_alloc_header_t),
      NULL,
      NULL);
  block->next = NULL;
  block->prev = NULL;
}

static inline void general_block_destroy(fstd_alloc_block_t *block) {
  if (block == NULL) {
    return;
  }

  general_block_destroy(block->next);
  if (block->next != NULL) {
    free(block->next);
  }

  free(block->storage);
}

void fstd_allocator_init(fstd_allocator_t *allocator, size_t block_size) {

  allocator->block_size = block_size;

  general_block_init(&allocator->base_block, block_size);

  allocator->last_block = &allocator->base_block;
}

void fstd_allocator_destroy(fstd_allocator_t *allocator) {
  general_block_destroy(&allocator->base_block);
}

void *fstd_alloc(fstd_allocator_t *allocator, uint32_t size) {
  if (size >= allocator->block_size + sizeof(fstd_alloc_header_t)) {
    return NULL;
  }

  fstd_alloc_header_t *best_header = NULL;
  fstd_alloc_block_t *block = allocator->last_block;
  bool can_insert_new_header = false;

  uint32_t padding = 0;
  if (size % 8 != 0) {
    padding = 8 - (size % 8);
  }

  while (block != NULL) {
    fstd_alloc_header_t *header = block->first_header;
    while (header != NULL) {
      if (!header->used && header->size >= size) {
        best_header = header;

        can_insert_new_header =
            header->size >= size + padding + sizeof(fstd_alloc_header_t);
        break;
      }

      header = header->next;
    }

    if (best_header != NULL) {
      break;
    }

    block = block->prev;
  }

  if (best_header == NULL) {
    fstd_alloc_block_t *new_block =
        (fstd_alloc_block_t *)malloc(sizeof(fstd_alloc_block_t));
    general_block_init(new_block, allocator->block_size);

    new_block->prev = allocator->last_block;

    allocator->last_block->next = new_block;
    allocator->last_block = new_block;

    return fstd_alloc(allocator, size);
  }

  if (can_insert_new_header) {
    // @NOTE: insert new header after the allocation
    uint32_t old_size = best_header->size;
    best_header->size = size + padding;
    fstd_alloc_header_t *new_header =
      (fstd_alloc_header_t *) ((uint8_t *)best_header +
                                     sizeof(fstd_alloc_header_t) +
                                     best_header->size);
    alloc_header_init(
        new_header,
        old_size - best_header->size - sizeof(fstd_alloc_header_t),
        best_header,
        best_header->next);
    best_header->next = new_header;
  }

  best_header->used = true;

  return best_header->addr;
}

void fstd_free(fstd_allocator_t *allocator, void *ptr) {
  fstd_alloc_header_t *header =
      (fstd_alloc_header_t *)(((uint8_t *)ptr) - sizeof(fstd_alloc_header_t));
  assert(header->addr == ptr);
  header->used = false;
  alloc_header_merge_if_necessary(header);
}

#endif // FSTD_ALLOC_IMPLEMENTATION

#ifdef __cplusplus
}
#endif

#endif
