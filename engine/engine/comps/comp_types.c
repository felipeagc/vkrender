#include "comp_types.h"
#include "gltf_comp.h"
#include "mesh_comp.h"
#include "point_light_comp.h"
#include "renderable_comp.h"
#include "terrain_comp.h"
#include "transform_comp.h"

#define E(t, initializer, inspector, destructor, name) sizeof(t),
const size_t EG_COMP_SIZES[] = {EG__COMPS};
#undef E

#define E(t, initializer, inspector, destructor, name) name,
const char *EG_COMP_NAMES[] = {EG__COMPS};
#undef E

#define E(t, initializer, inspector, destructor, name)                         \
  ((eg_comp_initializer_t)initializer),
const eg_comp_initializer_t EG_COMP_INITIALIZERS[] = {EG__COMPS};
#undef E

#define E(t, initializer, inspector, destructor, name)                         \
  ((eg_comp_inspector_t)inspector),
const eg_comp_inspector_t EG_COMP_INSPECTORS[] = {EG__COMPS};
#undef E

#define E(t, initializer, inspector, destructor, name)                         \
  ((eg_comp_destructor_t)destructor),
const eg_comp_destructor_t EG_COMP_DESTRUCTORS[] = {EG__COMPS};
#undef E

#define E(enum_name, name) name,
const char *EG_TAG_NAMES[] = {EG__TAGS};
#undef E
