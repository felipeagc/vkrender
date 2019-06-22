#pragma once

#include <stdint.h>
#include <stddef.h>

typedef uint64_t re_hash_t;

typedef struct re_hasher_t {
  re_hash_t hash;
} re_hasher_t;

re_hasher_t re_hasher_create();

re_hash_t re_hasher_get(re_hasher_t *hasher);

void re_hash_data(re_hasher_t *hasher, void *data_, size_t size);
void re_hash_i32(re_hasher_t *hasher, int32_t data);
void re_hash_u32(re_hasher_t *hasher, uint32_t data);
void re_hash_f32(re_hasher_t *hasher, float data);
void re_hash_u64(re_hasher_t *hasher, uint64_t data);
void re_hash_string(re_hasher_t *hasher, const char *str);
