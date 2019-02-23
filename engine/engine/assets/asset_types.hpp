#pragma once

#include <stddef.h>

typedef void (*eg_asset_destructor_t)(void *);

typedef enum {
  EG_ENVIRONMENT_ASSET_TYPE,
  EG_MESH_ASSET_TYPE,
  EG_PBR_MATERIAL_ASSET_TYPE,
  EG_ASSET_TYPE_COUNT,
} eg_asset_type_t;

extern eg_asset_destructor_t eg_asset_destructors[EG_ASSET_TYPE_COUNT];

#define EG_REGISTER_ASSET(asset)                                               \
  {                                                                            \
    eg_asset_destructors[EG_ASSET_TYPE(asset)] =                               \
        (eg_asset_destructor_t)EG_ASSET_DESTROY(asset);                        \
  }

#define EG_ASSET_TYPE(asset) EG_ASSET_TYPE_##asset
#define EG_ASSET_INIT(asset) EG_ASSET_INIT_##asset
#define EG_ASSET_DESTROY(asset) EG_ASSET_DESTROY_##asset

#define EG_ASSET_TYPE_eg_environment_asset_t EG_ENVIRONMENT_ASSET_TYPE
#define EG_ASSET_INIT_eg_environment_asset_t eg_environment_asset_init
#define EG_ASSET_DESTROY_eg_environment_asset_t eg_environment_asset_destroy

#define EG_ASSET_TYPE_eg_mesh_asset_t EG_MESH_ASSET_TYPE
#define EG_ASSET_INIT_eg_mesh_asset_t eg_mesh_asset_init
#define EG_ASSET_DESTROY_eg_mesh_asset_t eg_mesh_asset_destroy

#define EG_ASSET_TYPE_eg_pbr_material_asset_t EG_PBR_MATERIAL_ASSET_TYPE
#define EG_ASSET_INIT_eg_pbr_material_asset_t eg_pbr_material_asset_init
#define EG_ASSET_DESTROY_eg_pbr_material_asset_t eg_pbr_material_asset_destroy

typedef struct eg_asset_t {
  eg_asset_type_t type;
  char *name;
} eg_asset_t;

void eg_asset_init(eg_asset_t *asset, eg_asset_type_t type);

void eg_asset_init_named(
    eg_asset_t *asset, eg_asset_type_t type, const char *name);

void eg_asset_destroy(eg_asset_t *asset);
