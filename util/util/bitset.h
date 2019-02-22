#ifndef UT_BITSET_H
#define UT_BITSET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define UT_BITSET(size)                                                        \
  struct {                                                                     \
    uint8_t bytes[(size + 7) / 8];                                             \
  }

inline bool ut_bitset_at(uint8_t *bitset, uint32_t pos) {
  uint32_t index = pos / 8;
  uint32_t bit = pos % 8;

  return bitset[index] & (1 << (bit));
}

inline void ut_bitset_set(uint8_t *bitset, uint32_t pos, bool val) {
  uint32_t index = pos / 8;
  uint32_t bit = pos % 8;
  bitset[index] = (val ? 1 : 0) << bit;
}

// Sets the bitset to zero
inline void ut_bitset_reset(uint8_t *bitset, uint32_t size) {
  for (uint32_t i = 0; i < size / 8; i++) {
    bitset[i] = 0;
  }
}

#ifdef __cplusplus
}
#endif

#endif
