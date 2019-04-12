#include "rendering_system.h"

#include "../components/gltf_model_component.h"
#include "../components/mesh_component.h"
#include "../components/transform_component.h"
#include "../world.h"
#include <renderer/pipeline.h>
#include <renderer/window.h>

void eg_rendering_system_render(
    const eg_cmd_info_t *cmd_info, eg_world_t *world, re_pipeline_t *pipeline) {
  re_cmd_bind_graphics_pipeline(cmd_info->cmd_buffer, pipeline);
  eg_camera_bind(&world->camera, cmd_info, pipeline, 0);
  eg_environment_bind(&world->environment, cmd_info, pipeline, 1);

  // Draw all meshes
  for (eg_entity_t entity = 0; entity < EG_MAX_ENTITIES; entity++) {
    if (eg_world_has_tag(world, entity, EG_ENTITY_TAG_HIDDEN)) {
      continue;
    }

    if (eg_world_has_comp(world, entity, EG_MESH_COMPONENT_TYPE) &&
        eg_world_has_comp(world, entity, EG_TRANSFORM_COMPONENT_TYPE)) {
      eg_mesh_component_t *mesh =
          EG_GET_COMP(world, entity, eg_mesh_component_t);
      eg_transform_component_t *transform =
          EG_GET_COMP(world, entity, eg_transform_component_t);

      mesh->model.uniform.transform = eg_transform_component_to_mat4(transform);
      eg_mesh_component_draw(mesh, cmd_info, pipeline);
    }

    if (eg_world_has_comp(world, entity, EG_GLTF_MODEL_COMPONENT_TYPE) &&
        eg_world_has_comp(world, entity, EG_TRANSFORM_COMPONENT_TYPE)) {
      eg_gltf_model_component_t *model =
          EG_GET_COMP(world, entity, eg_gltf_model_component_t);
      eg_transform_component_t *transform =
          EG_GET_COMP(world, entity, eg_transform_component_t);

      eg_gltf_model_component_draw(
          model, cmd_info, pipeline, eg_transform_component_to_mat4(transform));
    }
  }
}
