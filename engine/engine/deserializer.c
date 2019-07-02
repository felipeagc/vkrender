#include "deserializer.h"

#include "asset_manager.h"
#include "entity_manager.h"
#include "scene.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void eg_deserializer_init(eg_deserializer_t *deserializer) {
  memset(deserializer, 0, sizeof(*deserializer));
}

void eg_deserializer_destroy(eg_deserializer_t *deserializer) {
  if (deserializer->buffer != NULL) {
    free(deserializer->buffer);
    deserializer->buffer = NULL;
  }
}

void eg_deserializer_load(eg_deserializer_t *deserializer, const char *path) {
  if (deserializer->buffer != NULL) {
    free(deserializer->buffer);
    deserializer->buffer = NULL;
  }
  deserializer->buffer_offset = 0;

  FILE *file = fopen(path, "rb");
  assert(file);

  fseek(file, 0, SEEK_END);
  deserializer->buffer_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  deserializer->buffer = malloc(deserializer->buffer_size);

  fread(deserializer->buffer, deserializer->buffer_size, 1, file);

  fclose(file);
}

void eg_deserialize_scene(
    eg_deserializer_t *deserializer,
    eg_scene_t *scene,
    eg_asset_manager_t *asset_manager,
    eg_entity_manager_t *entity_manager) {
  deserializer->asset_manager  = asset_manager;
  deserializer->entity_manager = entity_manager;

  uint32_t asset_count  = eg_deserializer_read_u32(deserializer);
  uint32_t entity_count = eg_deserializer_read_u32(deserializer);

  for (uint32_t i = 0; i < asset_count; i++) {
    eg_asset_type_t type = eg_deserializer_read_u32(deserializer);
    eg_asset_uid_t uid   = eg_deserializer_read_u32(deserializer);
    char *name           = eg_deserializer_read_string(deserializer);

    eg_asset_t *asset = eg_asset_manager_alloc_uid(asset_manager, type, uid);
    eg_asset_set_name(asset, name);

    EG_ASSET_DESERIALIZERS[type](asset, deserializer);
  }

  eg_scene_deserialize(scene, deserializer);

  for (uint32_t i = 0; i < entity_count; i++) {
    eg_entity_t entity = eg_entity_add(entity_manager);

    uint64_t tags       = eg_deserializer_read_u64(deserializer);
    uint32_t comp_count = eg_deserializer_read_u32(deserializer);

    entity_manager->tags[entity] = tags;

    for (uint32_t j = 0; j < comp_count; j++) {
      eg_comp_type_t comp_type = eg_deserializer_read_u32(deserializer);

      void *comp = eg_comp_add(entity_manager, entity, comp_type);
      EG_COMP_DESERIALIZERS[comp_type](comp, deserializer);
    }
  }

  deserializer->asset_manager  = NULL;
  deserializer->entity_manager = NULL;

  // Make sure we've read the whole file
  assert(deserializer->buffer_offset == deserializer->buffer_size);
}

void eg_deserializer_read(
    eg_deserializer_t *deserializer, void *data, size_t size) {
  assert(deserializer->buffer_offset + size <= deserializer->buffer_size);

  memcpy(data, deserializer->buffer + deserializer->buffer_offset, size);
  deserializer->buffer_offset += size;
}

char *eg_deserializer_read_string(eg_deserializer_t *deserializer) {
  uint32_t byte_length = eg_deserializer_read_u32(deserializer);

  char *str = (char *)deserializer->buffer + deserializer->buffer_offset;
  deserializer->buffer_offset += (size_t)byte_length;

  return str;
}

uint32_t eg_deserializer_read_u32(eg_deserializer_t *deserializer) {
  uint32_t value = 0;
  eg_deserializer_read(deserializer, &value, sizeof(value));
  return value;
}

uint64_t eg_deserializer_read_u64(eg_deserializer_t *deserializer) {
  uint64_t value = 0;
  eg_deserializer_read(deserializer, &value, sizeof(value));
  return value;
}
