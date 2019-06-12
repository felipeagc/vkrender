#include "asset_types.h"

#include "gltf_asset.h"
#include "image_asset.h"
#include "mesh_asset.h"
#include "pbr_material_asset.h"
#include "pipeline_asset.h"
#include <string.h>

const char *const EG_DEFAULT_ASSET_NAME = "Unnamed asset";

#define E(t, constructor, inspector, destructor, name) sizeof(t),
const size_t EG_ASSET_SIZES[] = {EG__ASSETS};
#undef E

#define E(t, constructor, inspector, destructor, name) name,
const char *EG_ASSET_NAMES[] = {EG__ASSETS};
#undef E

#define E(t, constructor, inspector, destructor, name)                         \
  ((eg_asset_constructor_t)constructor),
const eg_asset_constructor_t EG_ASSET_CONSTRUCTORS[EG_ASSET_TYPE_MAX] = {
    EG__ASSETS};
#undef E

#define E(t, constructor, inspector, destructor, name)                         \
  ((eg_asset_inspector_t)inspector),
const eg_asset_inspector_t EG_ASSET_INSPECTORS[] = {EG__ASSETS};
#undef E

#define E(t, constructor, inspector, destructor, name)                         \
  ((eg_asset_destructor_t)destructor),
const eg_asset_destructor_t EG_ASSET_DESTRUCTORS[] = {EG__ASSETS};
#undef E

void eg_asset_set_name(eg_asset_t *asset, const char *name) {
  if (asset->name != NULL) {
    free(asset->name);
  }

  if (name == NULL || strlen(name) <= 0) {
    asset->name = NULL;
    return;
  }

  asset->name = strdup(name);
}

const char *eg_asset_get_name(eg_asset_t *asset) {
  if (asset->name == NULL) {
    return EG_DEFAULT_ASSET_NAME;
  }

  return asset->name;
}
