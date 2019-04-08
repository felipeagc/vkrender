#include "filesystem.h"
#include <physfs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int eg_mount(const char *path_to_archive, const char *mount_point) {
  const char *base_dir = PHYSFS_getBaseDir();

  size_t path_size = strlen(path_to_archive) + strlen(base_dir) + 2;
  path_size = path_size > strlen(path_to_archive)+7 ?
	  path_size : strlen(path_to_archive)+7;
  char *path = malloc(path_size);

  snprintf(path, path_size, "%s/%s", base_dir, path_to_archive);

  int res = PHYSFS_mount(path, mount_point, 1);
  if (!res) {
    res = PHYSFS_mount(path_to_archive, mount_point, 1);
  }
  if (!res) {
    snprintf(path, strlen(path_to_archive)+4, "../%s", path_to_archive);
    res = PHYSFS_mount(path, mount_point, 1);
  }
  if (!res) {
    snprintf(path, strlen(path_to_archive)+7, "../../%s", path_to_archive);
    res = PHYSFS_mount(path, mount_point, 1);
  }

  free(path);

  return res;
}

eg_file_t *eg_file_open_read(const char *path) {
  return (eg_file_t *)PHYSFS_openRead(path);
}

int eg_file_close(eg_file_t *file) { return PHYSFS_close((PHYSFS_File *)file); }

size_t eg_file_size(eg_file_t *file) {
  return (size_t)PHYSFS_fileLength((PHYSFS_File *)file);
}

size_t eg_file_read_bytes(eg_file_t *file, void *buffer, size_t length) {
  return (size_t)PHYSFS_readBytes((PHYSFS_File *)file, buffer, length);
}
