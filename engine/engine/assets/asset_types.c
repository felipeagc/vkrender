#include "asset_types.h"
#include "environment_asset.h"
#include "gltf_model_asset.h"
#include "mesh_asset.h"
#include "pbr_material_asset.h"
#include "pipeline_asset.h"
#include <stdlib.h>
#include <string.h>

eg_asset_destructor_t eg_asset_destructors[EG_ASSET_TYPE_COUNT] = {
    [EG_PIPELINE_ASSET_TYPE] = (eg_asset_destructor_t)eg_pipeline_asset_destroy,
    [EG_ENVIRONMENT_ASSET_TYPE] =
        (eg_asset_destructor_t)eg_environment_asset_destroy,
    [EG_MESH_ASSET_TYPE] = (eg_asset_destructor_t)eg_mesh_asset_destroy,
    [EG_PBR_MATERIAL_ASSET_TYPE] =
        (eg_asset_destructor_t)eg_pbr_material_asset_destroy,
    [EG_GLTF_MODEL_ASSET_TYPE] =
        (eg_asset_destructor_t)eg_gltf_model_asset_destroy,
};

void eg_asset_init(eg_asset_t *asset, eg_asset_type_t type, const char *name) {
  asset->type = type;
  asset->name = malloc(strlen(name) + 1);
  strncpy(asset->name, name, strlen(name) + 1);
}

void eg_asset_destroy(eg_asset_t *asset) {
  if (asset->name != NULL) {
    free(asset->name);
  }
}
