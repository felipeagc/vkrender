#pragma once

#include <cstddef>
#include <cstdint>

namespace ftl {

struct stack_allocator {
  struct block {
    block(size_t size) { this->storage = new uint8_t[size]; }

    ~block() {
      if (next_block != nullptr) {
        delete next_block;
      }
      if (storage != nullptr) {
        delete[] storage;
      }
    }

    uint8_t *storage = nullptr;
    size_t filled = 0;
    struct block *next_block = nullptr;
  };

  stack_allocator(size_t block_size = 16384) : m_base_block(block_size) {
    m_block_size = block_size;
  }

  void *alloc(size_t size, size_t alignment) {
    // For ensuring aligned memory allocations
    if (last_block->filled % alignment != 0) {
      last_block->filled += alignment - (last_block->filled % alignment);
    }

    if (last_block->filled + size > m_block_size) {
      last_block->next_block = new block(m_block_size);
      last_block = last_block->next_block;
    }

    void *ptr = (void *)&last_block->storage[last_block->filled];

    last_block->filled += size;

    return ptr;
  }

private:
  block m_base_block;
  block *last_block = &m_base_block;
  size_t m_block_size;
};

} // namespace ftl
