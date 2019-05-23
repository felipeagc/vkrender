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

#if defined(_MSC_VER)
#define FSTD__ALLOC_ALIGNAS(x) __declspec(align(x))
#elif defined(__clang__)
#define FSTD__ALLOC_ALIGNAS(x) __attribute__((aligned(x)))
#elif defined(__GNUC__)
#define FSTD__ALLOC_ALIGNAS(x) __attribute__((aligned(x)))
#endif

#define FSTD__ALLOC_ALIGNMENT 16

typedef FSTD__ALLOC_ALIGNAS(FSTD__ALLOC_ALIGNMENT) struct fstd_alloc_header_t {
  size_t size;
  size_t used;
  struct fstd_alloc_header_t *prev;
  struct fstd_alloc_header_t *next;
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

void *fstd_alloc(fstd_allocator_t *allocator, size_t size);

void *fstd_realloc(fstd_allocator_t *allocator, void *ptr, size_t size);

void fstd_free(fstd_allocator_t *allocator, void *ptr);

#ifdef FSTD_ALLOC_IMPLEMENTATION

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define FSTD__HEADER_ADDR(header)                                              \
  (((uint8_t *)header) + sizeof(fstd_alloc_header_t))

static inline void header_init(
    fstd_alloc_header_t *header, size_t size, fstd_alloc_header_t *prev,
    fstd_alloc_header_t *next) {
  header->prev = prev;
  header->next = next;
  header->size = size;
  header->used = false;
}

static inline void header_merge_if_necessary(fstd_alloc_header_t *header) {
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

static inline void block_init(fstd_alloc_block_t *block, size_t block_size) {
  assert(block_size > sizeof(fstd_alloc_header_t));
  block->storage = (uint8_t *)malloc(block_size);
  block->first_header = (fstd_alloc_header_t *)block->storage;
  header_init(
      block->first_header, block_size - sizeof(fstd_alloc_header_t), NULL,
      NULL);
  block->next = NULL;
  block->prev = NULL;
}

static inline void block_destroy(fstd_alloc_block_t *block) {
  if (block == NULL) {
    return;
  }

  block_destroy(block->next);
  if (block->next != NULL) {
    free(block->next);
  }

  free(block->storage);
}

void fstd_allocator_init(fstd_allocator_t *allocator, size_t block_size) {
  allocator->block_size = block_size;

  block_init(&allocator->base_block, block_size);

  allocator->last_block = &allocator->base_block;
}

void fstd_allocator_destroy(fstd_allocator_t *allocator) {
  block_destroy(&allocator->base_block);
}

void *fstd_alloc(fstd_allocator_t *allocator, size_t size) {
  if (size > allocator->block_size - sizeof(fstd_alloc_header_t)) {
    return NULL;
  }

  fstd_alloc_header_t *best_header = NULL;
  fstd_alloc_block_t *block = allocator->last_block;
  bool can_insert_new_header = false;

  while (block != NULL) {
    fstd_alloc_header_t *header = block->first_header;
    while (header != NULL) {
      if (!header->used && header->size >= size) {
        best_header = header;

        size_t padding = 0;
        size_t size_to_pad =
            ((uintptr_t)header) + sizeof(fstd_alloc_header_t) + size;
        if (size_to_pad % FSTD__ALLOC_ALIGNMENT != 0) {
          padding =
              FSTD__ALLOC_ALIGNMENT - (size_to_pad % FSTD__ALLOC_ALIGNMENT);
        }

        can_insert_new_header =
            (header->size >= sizeof(fstd_alloc_header_t) + size + padding);
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
    block_init(new_block, allocator->block_size);

    new_block->prev = allocator->last_block;

    allocator->last_block->next = new_block;
    allocator->last_block = new_block;

    return fstd_alloc(allocator, size);
  }

  if (can_insert_new_header) {
    size_t padding = 0;
    size_t size_to_pad =
        ((uintptr_t)best_header) + sizeof(fstd_alloc_header_t) + size;
    if (size_to_pad % FSTD__ALLOC_ALIGNMENT != 0) {
      padding = FSTD__ALLOC_ALIGNMENT - (size_to_pad % FSTD__ALLOC_ALIGNMENT);
    }

    // @NOTE: insert new header after the allocation
    size_t old_size = best_header->size;
    best_header->size = size + padding;
    fstd_alloc_header_t *new_header =
      (fstd_alloc_header_t *) ((uint8_t *)best_header +
                                     sizeof(fstd_alloc_header_t) +
                                     best_header->size);
    header_init(
        new_header, old_size - best_header->size - sizeof(fstd_alloc_header_t),
        best_header, best_header->next);
    best_header->next = new_header;
  }

  best_header->used = true;

  return FSTD__HEADER_ADDR(best_header);
}

void *fstd_realloc(fstd_allocator_t *allocator, void *ptr, size_t size) {
  if (ptr == NULL) {
    return fstd_alloc(allocator, size);
  }

  fstd_alloc_header_t *header =
      (fstd_alloc_header_t *)(((uint8_t *)ptr) - sizeof(fstd_alloc_header_t));

  if (header->size >= size) {
    // Already big enough
    header->used = true;
    return ptr;
  }

  fstd_alloc_header_t *next_header = header->next;
  size_t next_sizes = header->size;
  while (next_header != NULL && !next_header->used) {
    if (next_sizes >= size) {
      break;
    }
    next_sizes += sizeof(fstd_alloc_header_t) + next_header->size;
    next_header = header->next;
  }

  if (next_sizes >= size) {
    // Grow header
    if (next_header->next != NULL) {
      next_header->next->prev = header;
    }
    header->next = next_header->next;
    header->size = next_sizes;
    header->used = true;

    return ptr;
  }

  void *new_ptr = fstd_alloc(allocator, size);
  memcpy(new_ptr, ptr, header->size);

  header->used = false;
  header_merge_if_necessary(header);

  return new_ptr;
}

void fstd_free(fstd_allocator_t *allocator, void *ptr) {
  fstd_alloc_header_t *header =
      (fstd_alloc_header_t *)(((uint8_t *)ptr) - sizeof(fstd_alloc_header_t));
  assert(FSTD__HEADER_ADDR(header) == ptr);
  header->used = false;
  header_merge_if_necessary(header);
}

#endif // FSTD_ALLOC_IMPLEMENTATION

#ifdef __cplusplus
}
#endif

#endif
