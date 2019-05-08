#include "light_system.h"
#include "../comps/point_light_comp.h"
#include "../comps/transform_comp.h"
#include "../environment.h"
#include "../world.h"

void eg_light_system(eg_world_t *world) {
  eg_transform_comp_t *transforms = EG_COMP_ARRAY(world, eg_transform_comp_t);
  eg_point_light_comp_t *point_lights =
      EG_COMP_ARRAY(world, eg_point_light_comp_t);

  eg_environment_reset_point_lights(&world->environment);

  for (eg_entity_t e = 0; e < world->entity_max; e++) {
    if (EG_HAS_TAG(world, e, EG_TAG_HIDDEN)) {
      continue;
    }

    if (EG_HAS_COMP(world, eg_point_light_comp_t, e) &&
        EG_HAS_COMP(world, eg_transform_comp_t, e)) {
      eg_environment_add_point_light(
          &world->environment,
          transforms[e].position,
          vec4_muls(point_lights[e].color, point_lights[e].intensity));
    }
  }
}
