#pragma once

#include <stddef.h>

/*
  To add a new asset type:
  1. Add it to the eg_asset_type_t enum
  2. Register it in asset_manager.c
  3. Make sure to have the first member in the specific asset struct be an
     eg_asset_t and to initialize/destroy it in the specific asset's
     initialization/destruction functions
 */

typedef void (*eg_asset_destructor_t)(void *);

typedef enum eg_asset_type_t {
  EG_ENVIRONMENT_ASSET_TYPE,
  EG_MESH_ASSET_TYPE,
  EG_PBR_MATERIAL_ASSET_TYPE,
  EG_GLTF_MODEL_ASSET_TYPE,
  EG_ASSET_TYPE_COUNT,
} eg_asset_type_t;

extern eg_asset_destructor_t eg_asset_destructors[EG_ASSET_TYPE_COUNT];

static inline void eg_register_asset(
    eg_asset_type_t asset_type, eg_asset_destructor_t destructor) {
  eg_asset_destructors[asset_type] = destructor;
}

typedef struct eg_asset_t {
  eg_asset_type_t type;
  char *name;
} eg_asset_t;

void eg_asset_init(eg_asset_t *asset, eg_asset_type_t type);

void eg_asset_init_named(
    eg_asset_t *asset, eg_asset_type_t type, const char *name);

void eg_asset_destroy(eg_asset_t *asset);
