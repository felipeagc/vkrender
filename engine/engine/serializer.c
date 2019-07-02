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
}

void eg_serializer_destroy(eg_serializer_t *serializer) {
  free(serializer->buffer);

  memset(serializer, 0, sizeof(*serializer));
}

void eg_serialize_scene(
    eg_serializer_t *serializer,
    eg_scene_t *scene,
    eg_asset_manager_t *asset_manager,
    eg_entity_manager_t *entity_manager) {
  serializer->buffer_offset = 0;

  uint32_t asset_count  = 0;
  uint32_t entity_count = 0;

  // First pass (to get counts)
  for (uint32_t i = 0; i < asset_manager->count; i++) {
    eg_asset_t *asset = eg_asset_manager_get(asset_manager, i);
    if (!asset) continue;

    asset_count += 1;
  }

  for (uint32_t e = 0; e < entity_manager->entity_max; e++) {
    if (!eg_entity_exists(entity_manager, e)) continue;

    entity_count += 1;
  }

  // Write asset count
  eg_serializer_append_u32(serializer, asset_count);
  // Write entity count
  eg_serializer_append_u32(serializer, entity_count);

  // Second pass (write the actual data)
  for (uint32_t i = 0; i < asset_manager->count; i++) {
    eg_asset_t *asset = eg_asset_manager_get(asset_manager, i);
    if (!asset) continue;

    // Type
    eg_serializer_append_u32(serializer, (uint32_t)asset->type);
    // UID
    eg_serializer_append_u32(serializer, (uint32_t)asset->uid);
    // Name
    eg_serializer_append_string(serializer, asset->name);

    EG_ASSET_SERIALIZERS[asset->type](asset, serializer);
  }

  eg_scene_serialize(scene, serializer);

  for (uint32_t e = 0; e < entity_manager->entity_max; e++) {
    if (!eg_entity_exists(entity_manager, e)) continue;

    uint32_t comp_count = 0;

    for (uint32_t c = 0; c < EG_COMP_TYPE_MAX; c++) {
      if (EG_HAS_COMP_ID(entity_manager, e, c)) {
        comp_count += 1;
      }
    }

    // Tags
    eg_serializer_append_u64(serializer, entity_manager->tags[e]);

    // Component count
    eg_serializer_append_u32(serializer, comp_count);

    printf("Entity %u (%u comps):\n", e, comp_count);

    for (uint32_t c = 0; c < EG_COMP_TYPE_MAX; c++) {
      if (!EG_HAS_COMP_ID(entity_manager, e, c)) continue;

      void *comp = EG_COMP_BY_ID(entity_manager, e, c);

      printf("  %s\n", EG_COMP_NAMES[c]);

      // Component type
      eg_serializer_append_u32(serializer, c);

      EG_COMP_SERIALIZERS[c](comp, serializer);
    }
  }
}

void eg_serializer_save(eg_serializer_t *serializer, const char *path) {
  FILE *file = fopen(path, "wb");
  assert(file);

  fwrite(serializer->buffer, serializer->buffer_offset, 1, file);

  fclose(file);
}

void eg_serializer_append(
    eg_serializer_t *serializer, void *data, size_t size) {
  if (serializer->buffer_offset + size >= serializer->buffer_size) {
    serializer->buffer_size *= 2;
    serializer->buffer_size += size;
    serializer->buffer = realloc(serializer->buffer, serializer->buffer_size);
  }

  memcpy(&serializer->buffer[serializer->buffer_offset], data, size);
  serializer->buffer_offset += size;
}

void eg_serializer_append_string(eg_serializer_t *serializer, char *string) {
  if (string == NULL) {
    uint32_t byte_length = 1;
    char null_char       = '\0';
    eg_serializer_append_u32(serializer, byte_length);
    eg_serializer_append(serializer, &null_char, byte_length);
    return;
  }

  uint32_t byte_length = strlen(string) + 1;
  eg_serializer_append_u32(serializer, byte_length);
  eg_serializer_append(serializer, string, byte_length);
}

void eg_serializer_append_u32(eg_serializer_t *serializer, uint32_t data) {
  eg_serializer_append(serializer, &data, sizeof(data));
}

void eg_serializer_append_u64(eg_serializer_t *serializer, uint64_t data) {
  eg_serializer_append(serializer, &data, sizeof(data));
}
