#ifndef FSTD_BITSET_H
#define FSTD_BITSET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define FSTD_BITSET(size)                                                      \
  struct {                                                                     \
    unsigned char bytes[(size + 7) / 8];                                       \
  }

static inline bool fstd_bitset_at(void *bitset, uint32_t pos) {
  uint32_t index = pos / 8;
  uint32_t bit = pos % 8;
  unsigned char *bytes = (unsigned char *)bitset;

  return bytes[index] & (1UL << bit);
}

static inline void fstd_bitset_set(void *bitset, uint32_t pos, bool val) {
  uint32_t index = pos / 8;
  uint32_t bit = pos % 8;
  unsigned char *bytes = (unsigned char *)bitset;

  bytes[index] ^= (-(unsigned char)val ^ bytes[index]) & (1UL << bit);
}

// Sets the bitset to zero
static inline void fstd_bitset_reset(void *bitset, uint32_t size) {
  unsigned char *bytes = (unsigned char *)bitset;
  for (uint32_t i = 0; i < size / 8; i++) {
    bytes[i] = 0UL;
  }
}

#ifdef __cplusplus
}
#endif

#endif
