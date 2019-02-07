#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>

char *ut_load_string_from_file(const char *path) {
  FILE *file = fopen(path, "r");
  if (file == NULL)
    return NULL;

  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *buffer = (char *)malloc(size + 1);

  fread(buffer, sizeof(char), size, file);

  buffer[size] = '\0';

  fclose(file);

  return buffer;
}

unsigned char *ut_load_bytes_from_file(const char *path, size_t *size) {
  FILE *file = fopen(path, "rb");
  if (file == NULL)
    return NULL;

  fseek(file, 0, SEEK_END);
  *size = ftell(file);
  fseek(file, 0, SEEK_SET);

  unsigned char *buffer = (unsigned char *)malloc(*size);

  fread(buffer, *size, 1, file);

  fclose(file);

  return buffer;
}

#ifdef __cplusplus
}
#endif
