#ifndef FSTD_MAP_H
#define FSTD_MAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef enum fstd__map_value_state_t {
  FSTD__MAP_VALUE_EMPTY,
  FSTD__MAP_VALUE_DELETED,
  FSTD__MAP_VALUE_FILLED,
} fstd__map_value_state_t;

typedef struct fstd_map_t {
  void *bundles;
  size_t capacity;
  size_t filled;
  size_t value_size;
  size_t value_offset;
  size_t bundle_size;
} fstd_map_t;

#define FSTD__BUNDLE(val_type)                                                 \
  struct {                                                                     \
    char *key;                                                                 \
    fstd__map_value_state_t state;                                             \
    val_type val;                                                              \
  }

#define fstd_map_init(map, capacity, val_type)                                 \
  fstd__map_init(                                                              \
      map,                                                                     \
      capacity,                                                                \
      sizeof(val_type),                                                        \
      offsetof(FSTD__BUNDLE(val_type), val),                                   \
      sizeof(FSTD__BUNDLE(val_type)))

void fstd__map_init(
    fstd_map_t *map,
    size_t capacity,
    size_t value_size,
    size_t value_offset,
    size_t bundle_size);

void *fstd_map_get(fstd_map_t *map, const char *key);

void *fstd_map_get(fstd_map_t *map, const char *key);

void *fstd_map_get_by_index(fstd_map_t *map, size_t index, char **key);

char *fstd_map_get_key(fstd_map_t *map, void *value);

void *fstd_map_set(fstd_map_t *map, const char *key, void *value);

void *fstd_map_remove(fstd_map_t *map, const char *key);

void fstd_map_destroy(fstd_map_t *map);

static inline size_t fstd__djb_hash(const char *str) {
  size_t hash = 5381;
  char c;

  while ((c = *str++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

  return hash;
}

#ifdef FSTD_MAP_IMPLEMENTATION

#define FSTD__MAP_BUNDLE(map, hash)                                            \
  (&((char *)map->bundles)[hash * map->bundle_size])

#define FSTD__MAP_BUNDLE_KEY(map, hash) ((char **)FSTD__MAP_BUNDLE(map, hash));

#define FSTD__MAP_BUNDLE_STATE(map, hash)                                      \
  ((fstd__map_value_state_t *)(FSTD__MAP_BUNDLE(map, hash) + sizeof(char *)));

#define FSTD__MAP_BUNDLE_VALUE(map, hash)                                      \
  (FSTD__MAP_BUNDLE(map, hash) + map->value_offset);

void fstd__map_init(
    fstd_map_t *map,
    size_t capacity,
    size_t value_size,
    size_t value_offset,
    size_t bundle_size) {
  map->capacity = capacity;
  map->filled = 0;
  map->value_size = value_size;
  map->value_offset = value_offset;
  map->bundle_size = bundle_size;

  map->bundles = calloc(map->capacity, map->bundle_size);
}

void *fstd_map_get(fstd_map_t *map, const char *key) {
  size_t hash = fstd__djb_hash(key);
  size_t index = hash % map->capacity;
  size_t index_start = index;

  char **bundle_key = FSTD__MAP_BUNDLE_KEY(map, index);
  fstd__map_value_state_t *bundle_state = FSTD__MAP_BUNDLE_STATE(map, index);
  char *bundle_value = FSTD__MAP_BUNDLE_VALUE(map, index);

  while (*bundle_state != FSTD__MAP_VALUE_FILLED ||
         (*bundle_key != NULL && strcmp(*bundle_key, key) != 0)) {
    index = (index + 1) % map->capacity;

    bundle_key = FSTD__MAP_BUNDLE_KEY(map, index);
    bundle_state = FSTD__MAP_BUNDLE_STATE(map, index);
    bundle_value = FSTD__MAP_BUNDLE_VALUE(map, index);

    if (index == index_start) {
      return NULL;
    }
  }

  return bundle_value;
}

void *fstd_map_get_by_index(fstd_map_t *map, size_t index, char **key) {
  fstd__map_value_state_t *bundle_state = FSTD__MAP_BUNDLE_STATE(map, index);

  if (*bundle_state != FSTD__MAP_VALUE_FILLED) {
    return NULL;
  }

  char **bundle_key = FSTD__MAP_BUNDLE_KEY(map, index);

  if (key != NULL) {
    *key = *bundle_key;
  }

  char *bundle_value = FSTD__MAP_BUNDLE_VALUE(map, index);

  return bundle_value;
}

char *fstd_map_get_key(fstd_map_t *map, void *value) {
  return *((char **)(((char *)value) - map->value_offset));
}

void *fstd_map_set(fstd_map_t *map, const char *key, void *value) {
  size_t hash = fstd__djb_hash(key);
  size_t index = hash % map->capacity;
  size_t index_start = index;

  char **bundle_key = FSTD__MAP_BUNDLE_KEY(map, index);
  fstd__map_value_state_t *bundle_state = FSTD__MAP_BUNDLE_STATE(map, index);
  char *bundle_value = FSTD__MAP_BUNDLE_VALUE(map, index);

  size_t first_deleted = SIZE_MAX;

  char *existing_value = NULL;

  while (*bundle_state != FSTD__MAP_VALUE_EMPTY) {
    // Collision!
    if (first_deleted == SIZE_MAX && *bundle_state == FSTD__MAP_VALUE_DELETED) {
      first_deleted = index;
    }

    if (*bundle_key != NULL && strcmp(*bundle_key, key) == 0) {
      existing_value = bundle_value;
      break;
    }

    index = (index + 1) % map->capacity;

    bundle_key = FSTD__MAP_BUNDLE_KEY(map, index);
    bundle_state = FSTD__MAP_BUNDLE_STATE(map, index);
    bundle_value = FSTD__MAP_BUNDLE_VALUE(map, index);

    if (index == index_start) {
      break;
    }
  }

  if (!existing_value && map->filled == map->capacity) {
    // Can't add a new element, we're at capacity
    return NULL;
  }

  if (!existing_value && first_deleted != SIZE_MAX) {
    bundle_key = FSTD__MAP_BUNDLE_KEY(map, first_deleted);
    bundle_state = FSTD__MAP_BUNDLE_STATE(map, first_deleted);
    bundle_value = FSTD__MAP_BUNDLE_VALUE(map, first_deleted);
  }

  *bundle_key = calloc(1, strlen(key) + 1);
  strncpy(*bundle_key, key, strlen(key) + 1);
  memcpy(bundle_value, value, map->value_size);

  if (*bundle_state != FSTD__MAP_VALUE_FILLED) {
    map->filled++;
    *bundle_state = FSTD__MAP_VALUE_FILLED;
  }

  return bundle_value;
}

void *fstd_map_remove(fstd_map_t *map, const char *key) {
  size_t hash = fstd__djb_hash(key);
  size_t index = hash % map->capacity;
  size_t index_start = index;

  char **bundle_key = FSTD__MAP_BUNDLE_KEY(map, index);
  fstd__map_value_state_t *bundle_state = FSTD__MAP_BUNDLE_STATE(map, index);
  char *bundle_value = FSTD__MAP_BUNDLE_VALUE(map, index);

  while (*bundle_key == NULL || strcmp(*bundle_key, key) != 0) {
    index = (index + 1) % map->capacity;

    bundle_key = FSTD__MAP_BUNDLE_KEY(map, index);
    bundle_state = FSTD__MAP_BUNDLE_STATE(map, index);
    bundle_value = FSTD__MAP_BUNDLE_VALUE(map, index);

    if (index == index_start) {
      return NULL;
    }
  }

  *bundle_state = FSTD__MAP_VALUE_DELETED;
  free(*bundle_key);
  *bundle_key = NULL;
  map->filled--;

  return bundle_value;
}

void fstd_map_destroy(fstd_map_t *map) {
  for (size_t i = 0; i < map->capacity; i++) {
    char **bundle_key = FSTD__MAP_BUNDLE_KEY(map, i);
    if (*bundle_key != NULL) {
      free(*bundle_key);
    }
  }

  free(map->bundles);
}

#endif // FSTD_MAP_IMPLEMENTATION

#ifdef __cplusplus
}
#endif

#endif
