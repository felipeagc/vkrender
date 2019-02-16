#include "asset.hpp"
#include <stdlib.h>
#include <string.h>

void eg_asset_init(eg_asset_t *asset, eg_asset_destructor_t destructor) {
  asset->name = NULL;
  asset->destructor = destructor;
}

void eg_asset_init_named(
    eg_asset_t *asset, eg_asset_destructor_t destructor, const char *name) {
  asset->destructor = destructor;
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
