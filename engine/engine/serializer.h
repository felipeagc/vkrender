#pragma once

#include "assets/asset_types.h"
#include "comps/comp_types.h"

typedef struct eg_scene_t eg_scene_t;
typedef struct eg_entity_manager_t eg_entity_manager_t;
typedef struct eg_asset_manager_t eg_asset_manager_t;

typedef struct eg_serializer_t {
  uint8_t *buffer;
  size_t buffer_offset;
  size_t buffer_size;
} eg_serializer_t;

void eg_serializer_init(eg_serializer_t *serializer);

void eg_serializer_destroy(eg_serializer_t *serializer);

void eg_serialize_scene(
    eg_serializer_t *serializer,
    eg_scene_t *scene,
    eg_asset_manager_t *asset_manager,
    eg_entity_manager_t *entity_manager);

void eg_serializer_save(eg_serializer_t *serializer, const char *path);

// Appends an asset header
void eg_serializer_add_asset(eg_serializer_t *serializer, eg_asset_t *asset);

// Appends an entity header
void eg_serializer_add_entity(eg_serializer_t *serializer, uint32_t comp_count);

// Appends a component header
void eg_serializer_add_comp(eg_serializer_t *serializer, eg_comp_type_t type);

void eg_serializer_append(eg_serializer_t *serializer, void *data, size_t size);

// Includes the null terminator
void eg_serializer_append_string(eg_serializer_t *serializer, char *string);
