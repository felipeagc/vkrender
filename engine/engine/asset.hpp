#pragma once

typedef void(*eg_asset_destructor_t)(void *);

typedef struct eg_asset_t {
  char *name;
  eg_asset_destructor_t destructor;
} eg_asset_t;

void eg_asset_init(eg_asset_t *asset, eg_asset_destructor_t destructor);

void eg_asset_init_named(
    eg_asset_t *asset, eg_asset_destructor_t destructor, const char *name);

void eg_asset_destroy(eg_asset_t *asset);
