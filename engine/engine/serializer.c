#include "serializer.h"

#include "asset_manager.h"
#include "entity_manager.h"
#include "scene.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_BUFFER_SIZE 1 << 16

void eg_serializer_init(eg_serializer_t *serializer) {
  memset(serializer, 0, sizeof(*serializer));

  serializer->buffer_size = INITIAL_BUFFER_SIZE;
  serializer->buffer = realloc(serializer->buffer, serializer->buffer_size);

  eg_scene_header_t header = {.asset_count = 0, .entity_count = 0};
  eg_serializer_append(serializer, &header, sizeof(header));
}

void eg_serializer_destroy(eg_serializer_t *serializer) {
  memset(serializer, 0, sizeof(*serializer));

  free(serializer->buffer);
}

void eg_serialize_scene(
    eg_serializer_t *serializer,
    eg_scene_t *scene,
    eg_asset_manager_t *asset_manager,
    eg_entity_manager_t *entity_manager) {
  for (uint32_t i = 0; i < asset_manager->count; i++) {
    eg_asset_t *asset = eg_asset_manager_get(asset_manager, i);
    if (!asset) continue;

    eg_serializer_add_asset(serializer, asset);
    EG_ASSET_SERIALIZERS[asset->type](asset, serializer);
  }

  for (uint32_t e = 0; e < entity_manager->entity_max; e++) {
    if (!eg_entity_exists(entity_manager, e)) continue;

    eg_serializer_add_entity(serializer);

    for (uint32_t c = 0; c < EG_COMP_TYPE_MAX; c++) {
      if (EG_HAS_COMP_ID(entity_manager, e, c)) {
        // TODO: serialize component
        // eg_serializer_add_comp(serializer, c, comp_data, size);
      }
    }
  }
}

void eg_serializer_save(eg_serializer_t *serializer, const char *path) {
  FILE *file = fopen(path, "wb");
  assert(file);

  fwrite(serializer->buffer, serializer->buffer_offset, 1, file);

  fclose(file);
}

void eg_serializer_add_asset(eg_serializer_t *serializer, eg_asset_t *asset) {
  eg_scene_header_t header;
  memcpy(&header, serializer->buffer, sizeof(header));
  header.asset_count += 1;
  memcpy(serializer->buffer, &header, sizeof(header));

  // Type
  eg_serializer_append(serializer, &asset->type, sizeof(asset->type));
  // UID
  eg_serializer_append(serializer, &asset->uid, sizeof(asset->uid));
  // Name
  eg_serializer_append_string(serializer, asset->name);
}

void eg_serializer_add_entity(eg_serializer_t *serializer) {
  eg_scene_header_t header;
  memcpy(&header, serializer->buffer, sizeof(header));
  header.entity_count += 1;
  memcpy(serializer->buffer, &header, sizeof(header));

  eg_entity_header_t e_header = {.comp_count = 0};

  // Entity header
  eg_serializer_append(serializer, &e_header, sizeof(e_header));

  serializer->cur_entity_offset = serializer->buffer_offset;
}

void eg_serializer_add_comp(eg_serializer_t *serializer, eg_comp_type_t type) {
  void *buffer_e_header = &serializer->buffer[serializer->cur_entity_offset];

  // Increment component count for current entity
  eg_entity_header_t e_header;
  memcpy(&e_header, buffer_e_header, sizeof(e_header));
  e_header.comp_count += 1;
  memcpy(buffer_e_header, &e_header, sizeof(e_header));

  eg_serializer_append(serializer, &type, sizeof(type));
}

void eg_serializer_append(
    eg_serializer_t *serializer, void *data, size_t size) {
  if (serializer->buffer_offset + size >= serializer->buffer_size) {
    serializer->buffer_size *= 2;
    serializer->buffer = realloc(serializer->buffer, serializer->buffer_size);
  }

  memcpy(&serializer->buffer[serializer->buffer_offset], data, size);
  serializer->buffer_offset += size;
}

void eg_serializer_append_string(eg_serializer_t *serializer, char *string) {
  uint32_t str_len = strlen(string);
  eg_serializer_append(serializer, &str_len, sizeof(str_len));
  eg_serializer_append(serializer, string, str_len);
}
