#pragma once

#include "assets/asset_types.h"
#include "comps/comp_types.h"

typedef struct eg_scene_t eg_scene_t;
typedef struct eg_entity_manager_t eg_entity_manager_t;
typedef struct eg_asset_manager_t eg_asset_manager_t;

typedef struct eg_scene_header_t {
  uint32_t asset_count;
  uint32_t entity_count;
} eg_scene_header_t;

typedef struct eg_entity_header_t {
  uint32_t comp_count;
} eg_entity_header_t;

typedef struct eg_serializer_t {
  uint8_t *buffer;
  size_t buffer_offset;
  size_t buffer_size;

  uint32_t cur_entity_offset;
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
void eg_serializer_add_entity(eg_serializer_t *serializer);

// Appends a component header
void eg_serializer_add_comp(eg_serializer_t *serializer, eg_comp_type_t type);

void eg_serializer_append(eg_serializer_t *serializer, void *data, size_t size);

void eg_serializer_append_string(eg_serializer_t *serializer, char *string);
