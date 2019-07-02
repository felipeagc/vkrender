#include "light_system.h"

#include "../comps/point_light_comp.h"
#include "../comps/transform_comp.h"
#include "../environment.h"
#include "../scene.h"

void eg_light_system(eg_scene_t *scene) {
  eg_entity_manager_t *entity_manager = &scene->entity_manager;

  eg_transform_comp_t *transforms =
      EG_COMP_ARRAY(entity_manager, eg_transform_comp_t);
  eg_point_light_comp_t *point_lights =
      EG_COMP_ARRAY(entity_manager, eg_point_light_comp_t);

  eg_environment_reset_point_lights(&scene->environment);

  for (eg_entity_t e = 0; e < entity_manager->entity_max; e++) {
    if (EG_HAS_TAG(entity_manager, e, EG_TAG_HIDDEN)) {
      continue;
    }

    if (EG_HAS_COMP(entity_manager, e, eg_point_light_comp_t) &&
        EG_HAS_COMP(entity_manager, e, eg_transform_comp_t)) {
      eg_environment_add_point_light(
          &scene->environment,
          transforms[e].position,
          vec4_muls(point_lights[e].color, point_lights[e].intensity));
    }
  }
}
