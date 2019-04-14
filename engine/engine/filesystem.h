#pragma once

#include <stddef.h>

typedef struct eg_file_t {
  void *opaque;
} eg_file_t;

// path_to_archive is relative to the directory the executable is in.
int eg_fs_mount(const char *path_to_archive, const char *mount_point);

eg_file_t *eg_file_open_read(const char *path);

int eg_file_close(eg_file_t *path);

size_t eg_file_size(eg_file_t *file);

size_t eg_file_read_bytes(eg_file_t *file, void *buffer, size_t length);

