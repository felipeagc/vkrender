#include "hasher.h"

re_hasher_t re_hasher_create() {
  return (re_hasher_t){.hash = 0xcbf29ce484222325ull};
}

re_hash_t re_hasher_get(re_hasher_t *hasher) { return hasher->hash; }

void re_hash_data(re_hasher_t *hasher, void *data_, size_t size) {
  unsigned char *data = (unsigned char *)data_;

  size /= sizeof(*data);
  for (size_t i = 0; i < size; i++) {
    hasher->hash = (hasher->hash * 0x100000001b3ull) ^ data[i];
  }
}

void re_hash_i32(re_hasher_t *hasher, int32_t data) {
  re_hash_u32(hasher, (uint32_t)data);
}

void re_hash_u32(re_hasher_t *hasher, uint32_t data) {
  hasher->hash = (hasher->hash * 0x100000001b3ull) ^ data;
}

void re_hash_u64(re_hasher_t *hasher, uint64_t data) {
  re_hash_u32(hasher, data & 0xffffffffu);
  re_hash_u32(hasher, data >> 32);
}

void re_hash_f32(re_hasher_t *hasher, float data) {
  union {
    float f32;
    uint32_t u32;
  } u;
  u.f32 = data;
  re_hash_u32(hasher, u.u32);
}

void re_hash_string(re_hasher_t *hasher, const char *str) {
  char c;
  re_hash_u32(hasher, 0xff);
  while ((c = *str++) != '\0') {
    re_hash_u32(hasher, (uint8_t)c);
  }
}
