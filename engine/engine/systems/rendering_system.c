#include "rendering_system.h"

#include "../assets/pipeline_asset.h"
#include "../comps/gltf_comp.h"
#include "../comps/mesh_comp.h"
#include "../comps/renderable_comp.h"
#include "../comps/transform_comp.h"
#include "../world.h"
#include <renderer/pipeline.h>
#include <renderer/window.h>

static void bind_stuff(
    eg_world_t *world, re_cmd_buffer_t *cmd_buffer, re_pipeline_t *pipeline) {
  if (pipeline == NULL) return;

  re_cmd_bind_pipeline(cmd_buffer, pipeline);
  eg_camera_bind(&world->camera, cmd_buffer, pipeline, 0);
  eg_environment_bind(&world->environment, cmd_buffer, pipeline, 1);
}

void eg_rendering_system(eg_world_t *world, re_cmd_buffer_t *cmd_buffer) {
  re_pipeline_t *pipeline = NULL;

  eg_transform_comp_t *transforms = EG_COMP_ARRAY(world, eg_transform_comp_t);
  eg_renderable_comp_t *renderables =
      EG_COMP_ARRAY(world, eg_renderable_comp_t);
  eg_mesh_comp_t *meshes      = EG_COMP_ARRAY(world, eg_mesh_comp_t);
  eg_gltf_comp_t *gltf_models = EG_COMP_ARRAY(world, eg_gltf_comp_t);

  // Draw all meshes
  for (eg_entity_t e = 0; e < world->entity_max; e++) {
    if (EG_HAS_TAG(world, e, EG_TAG_HIDDEN)) {
      continue;
    }

    if (!EG_HAS_COMP(world, eg_renderable_comp_t, e)) {
      continue;
    }

    // Bind pipeline
    re_pipeline_t *renderable_pipeline =
        eg_renderable_comp_get_pipeline(&renderables[e]);

    if (renderable_pipeline == NULL) {
      continue;
    }

    if (renderable_pipeline != pipeline) {
      pipeline = renderable_pipeline;
      bind_stuff(world, cmd_buffer, pipeline);
    }

    // Render mesh
    if (EG_HAS_COMP(world, eg_mesh_comp_t, e) &&
        EG_HAS_COMP(world, eg_transform_comp_t, e)) {

      if (EG_HAS_COMP(world, eg_terrain_comp_t, e)) {
        struct {
          float time;
        } pc;

        pc.time = (float)glfwGetTime();

        re_cmd_push_constants(cmd_buffer, pipeline, 0, sizeof(pc), &pc);
      }

      eg_mesh_comp_draw(
          &meshes[e],
          cmd_buffer,
          pipeline,
          eg_transform_comp_mat4(&transforms[e]));
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
