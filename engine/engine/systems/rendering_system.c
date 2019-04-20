#include "rendering_system.h"

#include "../components/gltf_model_component.h"
#include "../components/mesh_component.h"
#include "../components/transform_component.h"
#include "../world.h"
#include <renderer/pipeline.h>
#include <renderer/window.h>

void eg_rendering_system_render(
    eg_world_t *world, const eg_cmd_info_t *cmd_info, re_pipeline_t *pipeline) {
  re_cmd_bind_graphics_pipeline(cmd_info->cmd_buffer, pipeline);
  eg_camera_bind(&world->camera, cmd_info, pipeline, 0);
  eg_environment_bind(&world->environment, cmd_info, pipeline, 1);

  eg_transform_component_t *transforms =
      EG_COMP_ARRAY(world, eg_transform_component_t);
  eg_mesh_component_t *meshes = EG_COMP_ARRAY(world, eg_mesh_component_t);
  eg_gltf_model_component_t *gltf_models =
      EG_COMP_ARRAY(world, eg_gltf_model_component_t);

  // Draw all meshes
  for (eg_entity_t e = 0; e < world->entity_max; e++) {
    if (EG_HAS_TAG(world, e, EG_TAG_HIDDEN)) {
      continue;
    }

    if (EG_HAS_COMP(world, eg_mesh_component_t, e) &&
        EG_HAS_COMP(world, eg_transform_component_t, e)) {
      meshes[e].model.uniform.transform =
          eg_transform_component_to_mat4(&transforms[e]);
      eg_mesh_component_draw(&meshes[e], cmd_info, pipeline);
    }

    if (EG_HAS_COMP(world, eg_gltf_model_component_t, e) &&
        EG_HAS_COMP(world, eg_transform_component_t, e)) {
      eg_gltf_model_component_draw(
          &gltf_models[e],
          cmd_info,
          pipeline,
          eg_transform_component_to_mat4(&transforms[e]));
    }
  }
}
