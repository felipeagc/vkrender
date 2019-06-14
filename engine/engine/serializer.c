#include "serializer.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_BUFFER_SIZE 1 << 16

static inline void
append(eg_serializer_t *serializer, void *data, size_t size) {
  if (serializer->buffer_offset + size >= serializer->buffer_size) {
    serializer->buffer_size *= 2;
    serializer->buffer = realloc(serializer->buffer, serializer->buffer_size);
  }

  memcpy(&serializer->buffer[serializer->buffer_offset], data, size);
  serializer->buffer_offset += size;
}

void eg_serializer_init(eg_serializer_t *serializer) {
  memset(serializer, 0, sizeof(*serializer));

  serializer->buffer_size = INITIAL_BUFFER_SIZE;
  serializer->buffer = realloc(serializer->buffer, serializer->buffer_size);

  eg_scene_header_t header = {.asset_count = 0, .entity_count = 0};
  append(serializer, &header, sizeof(header));
}

void eg_serializer_destroy(eg_serializer_t *serializer) {
  memset(serializer, 0, sizeof(*serializer));

  free(serializer->buffer);
}

void eg_serializer_save(eg_serializer_t *serializer, const char *path) {
  FILE *file = fopen(path, "wb");
  assert(file);

  fwrite(serializer->buffer, serializer->buffer_offset, 1, file);

  fclose(file);
}

void eg_serializer_add_asset(
    eg_serializer_t *serializer,
    eg_asset_type_t type,
    void *asset_data,
    size_t size) {
  eg_scene_header_t header;
  memcpy(&header, serializer->buffer, sizeof(header));
  header.asset_count += 1;
  memcpy(serializer->buffer, &header, sizeof(header));

  eg_asset_header_t a_header = {.type = type, .size = size};
  append(serializer, &a_header, sizeof(a_header));

  append(serializer, asset_data, size);
}

void eg_serializer_add_entity(eg_serializer_t *serializer) {
  eg_scene_header_t header;
  memcpy(&header, serializer->buffer, sizeof(header));
  header.entity_count += 1;
  memcpy(serializer->buffer, &header, sizeof(header));

  eg_entity_header_t e_header = {.comp_count = 0};
  append(serializer, &e_header, sizeof(e_header));

  serializer->cur_entity_offset = serializer->buffer_offset;
}

void eg_serializer_add_comp(
    eg_serializer_t *serializer,
    eg_comp_type_t type,
    void *comp_data,
    size_t size) {
  eg_entity_header_t e_header;
  memcpy(
      &e_header,
      &serializer->buffer[serializer->cur_entity_offset],
      sizeof(e_header));

  e_header.comp_count += 1;

  memcpy(
      &serializer->buffer[serializer->cur_entity_offset],
      &e_header,
      sizeof(e_header));

  eg_comp_header_t c_header = {.type = type, .size = size};
  append(serializer, &c_header, sizeof(c_header));
  append(serializer, comp_data, size);
}
