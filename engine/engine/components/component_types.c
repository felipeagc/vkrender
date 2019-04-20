#include "component_types.h"
#include "gltf_model_component.h"
#include "mesh_component.h"
#include "transform_component.h"

#define E(t, destructor, name) sizeof(t),
const size_t EG_COMP_SIZES[] = {EG__COMPS};
#undef E

#define E(t, destructor, name) name,
const char *EG_COMP_NAMES[] = {EG__COMPS};
#undef E

#define E(t, destructor, name) ((eg_component_destructor_t)destructor),
const eg_component_destructor_t EG_COMP_DESTRUCTORS[] = {EG__COMPS};
#undef E

#define E(enum_name, name) name,
const char *EG_TAG_NAMES[] = {EG__TAGS};
#undef E
