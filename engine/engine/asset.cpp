#include "asset.hpp"
#include <stdlib.h>
#include <string.h>

void eg_asset_init(eg_asset_t *asset) { asset->name = NULL; }

void eg_asset_init_named(eg_asset_t *asset, const char *name) {
  if (strlen(name) == 0) {
    asset->name = NULL;
  } else {
    asset->name = (char *)malloc(strlen(name) + 1);
    strcpy(asset->name, name);
  }
}

void eg_asset_destroy(eg_asset_t *asset) {
  if (asset->name != NULL) {
    free(asset->name);
  }
}
