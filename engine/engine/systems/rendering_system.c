#include "rendering_system.h"

#include "../comps/gltf_comp.h"
#include "../comps/mesh_comp.h"
#include "../comps/renderable_comp.h"
#include "../comps/transform_comp.h"
#include "../world.h"
#include <renderer/pipeline.h>
#include <renderer/window.h>

static void bind_stuff(
    eg_world_t *world, re_cmd_buffer_t *cmd_buffer, re_pipeline_t *pipeline) {
  if (pipeline == NULL)
    return;

  re_cmd_bind_pipeline(cmd_buffer, pipeline);
  eg_camera_bind(&world->camera, cmd_buffer, pipeline, 0);
  eg_environment_bind(&world->environment, cmd_buffer, pipeline, 1);
}

void eg_rendering_system(eg_world_t *world, re_cmd_buffer_t *cmd_buffer) {
  re_pipeline_t *pipeline = NULL;

  eg_transform_comp_t *transforms = EG_COMP_ARRAY(world, eg_transform_comp_t);
  eg_renderable_comp_t *renderables =
      EG_COMP_ARRAY(world, eg_renderable_comp_t);
  eg_mesh_comp_t *meshes = EG_COMP_ARRAY(world, eg_mesh_comp_t);
  eg_gltf_comp_t *gltf_models = EG_COMP_ARRAY(world, eg_gltf_comp_t);

  // Draw all meshes
  for (eg_entity_t e = 0; e < world->entity_max; e++) {
    if (EG_HAS_TAG(world, e, EG_TAG_HIDDEN)) {
      continue;
    }

    // Bind pipeline
    if (EG_HAS_COMP(world, eg_renderable_comp_t, e)) {
      if (&renderables[e].pipeline->pipeline != pipeline) {
        pipeline = &renderables[e].pipeline->pipeline;
        bind_stuff(world, cmd_buffer, pipeline);
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

      if (EG_HAS_COMP(world, eg_terrain_comp_t, e)) {
        struct {
          float time;
        } pc;

        pc.time = (float)glfwGetTime();

        vkCmdPushConstants(
            cmd_buffer->cmd_buffer,
            pipeline->layout.layout,
            pipeline->layout.push_constants[0].stageFlags,
            pipeline->layout.push_constants[0].offset,
            sizeof(pc),
            &pc);
      }

      meshes[e].uniform.model = eg_transform_comp_mat4(&transforms[e]);
      eg_mesh_comp_draw(&meshes[e], cmd_buffer, pipeline);
    }

    if (EG_HAS_COMP(world, eg_gltf_comp_t, e) &&
        EG_HAS_COMP(world, eg_transform_comp_t, e)) {
      eg_gltf_comp_draw(
          &gltf_models[e],
          cmd_buffer,
          pipeline,
          eg_transform_comp_mat4(&transforms[e]));
    }
  }
}
