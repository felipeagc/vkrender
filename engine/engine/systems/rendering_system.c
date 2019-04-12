#include "rendering_system.h"

#include "../components/gltf_model_component.h"
#include "../components/mesh_component.h"
#include "../components/transform_component.h"
#include "../world.h"
#include <renderer/pipeline.h>
#include <renderer/window.h>

void eg_rendering_system_render(
    re_window_t *window,
    eg_world_t *world,
    VkCommandBuffer command_buffer,
    re_pipeline_t *pipeline) {
  re_pipeline_bind_graphics(pipeline, window);
  eg_camera_bind(&world->camera, window, command_buffer, pipeline, 0);
  eg_environment_bind(&world->environment, window, pipeline, 1);

  // Draw all meshes
  for (eg_entity_t entity = 0; entity < EG_MAX_ENTITIES; entity++) {
    if (eg_world_has_comp(world, entity, EG_MESH_COMPONENT_TYPE) &&
        eg_world_has_comp(world, entity, EG_TRANSFORM_COMPONENT_TYPE)) {
      eg_mesh_component_t *mesh =
          EG_GET_COMP(world, entity, eg_mesh_component_t);
      eg_transform_component_t *transform =
          EG_GET_COMP(world, entity, eg_transform_component_t);

      mesh->model.uniform.transform = eg_transform_component_to_mat4(transform);
      eg_mesh_component_draw(mesh, window, pipeline);
    }

    if (eg_world_has_comp(world, entity, EG_GLTF_MODEL_COMPONENT_TYPE) &&
        eg_world_has_comp(world, entity, EG_TRANSFORM_COMPONENT_TYPE)) {
      eg_gltf_model_component_t *model =
          EG_GET_COMP(world, entity, eg_gltf_model_component_t);
      eg_transform_component_t *transform =
          EG_GET_COMP(world, entity, eg_transform_component_t);

      eg_gltf_model_component_draw(
          model, window, pipeline, eg_transform_component_to_mat4(transform));
    }
  }
}
