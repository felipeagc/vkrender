#include "asset_types.h"

#include "environment_asset.h"
#include "gltf_asset.h"
#include "mesh_asset.h"
#include "pbr_material_asset.h"
#include "pipeline_asset.h"

#define E(t, initializer, inspector, destructor, name) sizeof(t),
const size_t EG_ASSET_SIZES[] = {EG__ASSETS};
#undef E

#define E(t, initializer, inspector, destructor, name) name,
const char *EG_ASSET_NAMES[] = {EG__ASSETS};
#undef E

#define E(t, initializer, inspector, destructor, name)                         \
  ((eg_comp_initializer_t)initializer),
/* const eg_asset_initializer_t EG_ASSET_INITIALIZERS[] = {EG__ASSETS}; */
#undef E

#define E(t, initializer, inspector, destructor, name)                         \
  ((eg_asset_inspector_t)inspector),
const eg_asset_inspector_t EG_ASSET_INSPECTORS[] = {EG__ASSETS};
#undef E

#define E(t, initializer, inspector, destructor, name)                         \
  ((eg_asset_destructor_t)destructor),
const eg_asset_destructor_t EG_ASSET_DESTRUCTORS[] = {EG__ASSETS};
#undef E
