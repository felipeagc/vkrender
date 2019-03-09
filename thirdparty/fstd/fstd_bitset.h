#ifndef FSTD_BITSET_H
#define FSTD_BITSET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define FSTD_BITSET(size)                                                      \
  struct {                                                                     \
    uint8_t bytes[(size + 7) / 8];                                             \
  }

static inline bool fstd_bitset_at(uint8_t *bitset, uint32_t pos) {
  uint32_t index = pos / 8;
  uint32_t bit = pos % 8;

  return (bitset[index] & (1UL << bit)) ? true : false;
}

static inline void fstd_bitset_set(uint8_t *bitset, uint32_t pos, bool val) {
  uint32_t index = pos / 8;
  uint32_t bit = pos % 8;
  bitset[index] |= (val ? 1UL : 0UL) << bit;
}

// Sets the bitset to zero
static inline void fstd_bitset_reset(uint8_t *bitset, uint32_t size) {
  for (uint32_t i = 0; i < size / 8; i++) {
    bitset[i] = 0;
  }
}

#ifdef __cplusplus
}
#endif

#endif
