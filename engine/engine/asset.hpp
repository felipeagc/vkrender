#pragma once

typedef struct eg_asset_t {
  char *name;
} eg_asset_t;

void eg_asset_init(eg_asset_t *asset);

void eg_asset_init_named(eg_asset_t *asset, const char *name);

void eg_asset_destroy(eg_asset_t *asset);
