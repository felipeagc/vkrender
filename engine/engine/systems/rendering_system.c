#include "rendering_system.h"

#include "../comps/gltf_comp.h"
#include "../comps/mesh_comp.h"
#include "../comps/renderable_comp.h"
#include "../comps/transform_comp.h"
#include "../world.h"
#include <renderer/pipeline.h>
#include <renderer/window.h>

static void bind_stuff(
    eg_world_t *world, const eg_cmd_info_t *cmd_info, re_pipeline_t *pipeline) {
  if (pipeline == NULL)
    return;

  re_cmd_bind_graphics_pipeline(cmd_info->cmd_buffer, pipeline);
  eg_camera_bind(&world->camera, cmd_info, pipeline, 0);
  eg_environment_bind(&world->environment, cmd_info, pipeline, 1);
}

void eg_rendering_system(eg_world_t *world, const eg_cmd_info_t *cmd_info) {
  re_pipeline_t *pipeline = NULL;

  eg_transform_comp_t *transforms = EG_COMP_ARRAY(world, eg_transform_comp_t);
  eg_renderable_comp_t *renderables =
      EG_COMP_ARRAY(world, eg_renderable_comp_t);
  eg_mesh_comp_t *meshes = EG_COMP_ARRAY(world, eg_mesh_comp_t);
  eg_gltf_comp_t *gltf_models =
      EG_COMP_ARRAY(world, eg_gltf_comp_t);

  // Draw all meshes
  for (eg_entity_t e = 0; e < world->entity_max; e++) {
    if (EG_HAS_TAG(world, e, EG_TAG_HIDDEN)) {
      continue;
    }

    // Bind pipeline
    if (EG_HAS_COMP(world, eg_renderable_comp_t, e)) {
      if (&renderables[e].pipeline->pipeline != pipeline) {
        pipeline = &renderables[e].pipeline->pipeline;
        bind_stuff(world, cmd_info, pipeline);
      }
    } else {
      pipeline = NULL;
    }

    if (pipeline == NULL) {
      continue;
    }

    // Render mesh
    if (EG_HAS_COMP(world, eg_mesh_comp_t, e) &&
        EG_HAS_COMP(world, eg_transform_comp_t, e)) {
      meshes[e].model.uniform.transform =
          eg_transform_comp_mat4(&transforms[e]);
      eg_mesh_comp_draw(&meshes[e], cmd_info, pipeline);
    }

    if (EG_HAS_COMP(world, eg_gltf_comp_t, e) &&
        EG_HAS_COMP(world, eg_transform_comp_t, e)) {
      eg_gltf_comp_draw(
          &gltf_models[e],
          cmd_info,
          pipeline,
          eg_transform_comp_mat4(&transforms[e]));
    }
  }
}
