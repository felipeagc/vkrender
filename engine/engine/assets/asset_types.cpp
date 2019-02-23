#include "asset_types.hpp"
#include <stdlib.h>
#include <string.h>

eg_asset_destructor_t eg_asset_destructors[EG_ASSET_TYPE_COUNT];

void eg_asset_init(eg_asset_t *asset, eg_asset_type_t type) {
  asset->type = type;
  asset->name = NULL;
}

void eg_asset_init_named(
    eg_asset_t *asset, eg_asset_type_t type, const char *name) {
  asset->type = type;
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
