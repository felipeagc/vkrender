#include "comp_types.h"
#include "gltf_comp.h"
#include "mesh_comp.h"
#include "point_light_comp.h"
#include "transform_comp.h"
#include "renderable_comp.h"

#define E(t, destructor, name) sizeof(t),
const size_t EG_COMP_SIZES[] = {EG__COMPS};
#undef E

#define E(t, destructor, name) name,
const char *EG_COMP_NAMES[] = {EG__COMPS};
#undef E

#define E(t, destructor, name) ((eg_comp_destructor_t)destructor),
const eg_comp_destructor_t EG_COMP_DESTRUCTORS[] = {EG__COMPS};
#undef E

#define E(enum_name, name) name,
const char *EG_TAG_NAMES[] = {EG__TAGS};
#undef E
