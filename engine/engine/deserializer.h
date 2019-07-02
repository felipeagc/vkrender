#pragma once

#include "assets/asset_types.h"
#include "comps/comp_types.h"

typedef struct eg_scene_t eg_scene_t;
typedef struct eg_entity_manager_t eg_entity_manager_t;
typedef struct eg_asset_manager_t eg_asset_manager_t;

typedef struct eg_deserializer_t {
  uint8_t *buffer;
  size_t buffer_offset;
  size_t buffer_size;

  eg_entity_manager_t *entity_manager;
  eg_asset_manager_t *asset_manager;
} eg_deserializer_t;

void eg_deserializer_init(eg_deserializer_t *deserializer);

void eg_deserializer_destroy(eg_deserializer_t *deserializer);

void eg_deserializer_load(eg_deserializer_t *deserializer, const char *path);

void eg_deserialize_scene(
    eg_deserializer_t *deserializer,
    eg_scene_t *scene,
    eg_asset_manager_t *asset_manager,
    eg_entity_manager_t *entity_manager);

void eg_deserializer_read(
    eg_deserializer_t *deserializer, void *data, size_t size);

char *eg_deserializer_read_string(eg_deserializer_t *deserializer);

uint32_t eg_deserializer_read_u32(eg_deserializer_t *deserializer);

uint64_t eg_deserializer_read_u64(eg_deserializer_t *deserializer);
