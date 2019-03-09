#include <engine/asset_manager.h>
#include <engine/assets/environment_asset.h>
#include <engine/assets/mesh_asset.h>
#include <engine/camera.h>
#include <engine/components/mesh_component.h>
#include <engine/components/transform_component.h>
#include <engine/engine.h>
#include <engine/inspector.h>
#include <engine/pbr.h>
#include <engine/pipelines.h>
#include <engine/systems/fps_camera_system.h>
#include <engine/world.h>
#include <fstd_util.h>
#include <renderer/renderer.h>
#include <util/log.h>

int main() {
  re_window_t window;
  re_window_init(&window, "Re-write", 1600, 900);
  re_imgui_init(&window);

  eg_engine_init();

  window.clear_color = (vec4_t){1.0, 1.0, 1.0, 1.0};

  re_pipeline_t pbr_pipeline;
  eg_init_pipeline_spv(
      &pbr_pipeline,
      window.render_target,
      "../shaders/out/pbr.vert.spv",
      "../shaders/out/pbr.frag.spv",
      eg_pbr_pipeline_parameters());

  re_pipeline_t skybox_pipeline;
  eg_init_pipeline_spv(
      &skybox_pipeline,
      window.render_target,
      "../shaders/out/skybox.vert.spv",
      "../shaders/out/skybox.frag.spv",
      eg_skybox_pipeline_parameters());

  eg_asset_manager_t asset_manager;
  eg_asset_manager_init(&asset_manager);

  eg_environment_asset_t *environment_asset =
      eg_asset_alloc(&asset_manager, eg_environment_asset_t);
  eg_environment_asset_init(
      environment_asset, "../assets/ice_lake/", 9, "../assets/brdf_lut.png");

  eg_world_t world;
  eg_world_init(&world, environment_asset);

  eg_fps_camera_system_t fps_system;
  eg_fps_camera_system_init(&fps_system);

  re_vertex_t vertices[] = {
      {{-1, -1, 0}, {0, 0, 1}, {0, 0}},
      {{1, -1, 0}, {0, 0, 1}, {1, 0}},
      {{1, 1, 0}, {0, 0, 1}, {1, 1}},
      {{-1, 1, 0}, {0, 0, 1}, {0, 1}},
  };

  uint32_t indices[] = {0, 1, 2, 2, 3, 0};

  eg_mesh_asset_t *mesh_asset = eg_asset_alloc(&asset_manager, eg_mesh_asset_t);
  eg_mesh_asset_init(
      mesh_asset, vertices, ARRAYSIZE(vertices), indices, ARRAYSIZE(indices));

  eg_pbr_material_asset_t *material =
      eg_asset_alloc(&asset_manager, eg_pbr_material_asset_t);
  eg_pbr_material_asset_init(material, NULL, NULL, NULL, NULL, NULL);

  {
    eg_entity_t ent = eg_world_add_entity(&world);
    eg_transform_component_t *transform_comp =
        (eg_transform_component_t *)eg_world_add_comp(
            &world, ent, EG_TRANSFORM_COMPONENT_TYPE);
    eg_transform_component_init(transform_comp);
    eg_mesh_component_t *mesh_comp = (eg_mesh_component_t *)eg_world_add_comp(
        &world, ent, EG_MESH_COMPONENT_TYPE);
    eg_mesh_component_init(mesh_comp, mesh_asset, material);
  }

  {
    eg_entity_t ent = eg_world_add_entity(&world);
    eg_transform_component_t *transform_comp =
        (eg_transform_component_t *)eg_world_add_comp(
            &world, ent, EG_TRANSFORM_COMPONENT_TYPE);
    eg_transform_component_init(transform_comp);
    eg_mesh_component_t *mesh_comp = (eg_mesh_component_t *)eg_world_add_comp(
        &world, ent, EG_MESH_COMPONENT_TYPE);
    eg_mesh_component_init(mesh_comp, mesh_asset, material);
  }

  while (!window.should_close) {
    SDL_Event event;
    while (re_window_poll_event(&window, &event)) {
      re_imgui_process_event(&event);
      eg_fps_camera_system_process_event(&fps_system, &window, &event);

      switch (event.type) {
      case SDL_QUIT:
        window.should_close = true;
        break;
      }
    }

    // Per-frame updates
    eg_environment_update(&world.environment, &window);

    re_imgui_begin(&window);

    eg_draw_inspector(&world, &asset_manager);

    re_imgui_end();

    re_window_begin_frame(&window);

    eg_fps_camera_system_update(&fps_system, &window, &world.camera);

    re_window_begin_render_pass(&window);

    eg_camera_bind(&world.camera, &window, &skybox_pipeline, 0);
    eg_environment_draw_skybox(&world.environment, &window, &skybox_pipeline);

    re_pipeline_bind_graphics(&pbr_pipeline, &window);
    eg_camera_bind(&world.camera, &window, &pbr_pipeline, 0);
    eg_environment_bind(&world.environment, &window, &pbr_pipeline, 1);

    // Draw all meshes
    for (eg_entity_t entity = 0; entity < EG_MAX_ENTITIES; entity++) {
      if (eg_world_has_comp(&world, entity, EG_MESH_COMPONENT_TYPE) &&
          eg_world_has_comp(&world, entity, EG_TRANSFORM_COMPONENT_TYPE)) {
        eg_mesh_component_t *mesh =
            EG_GET_COMP(&world, entity, eg_mesh_component_t);
        eg_transform_component_t *transform =
            EG_GET_COMP(&world, entity, eg_transform_component_t);

        mesh->model.uniform.transform =
            eg_transform_component_to_mat4(transform);
        eg_mesh_component_draw(mesh, &window, &pbr_pipeline);
      }
    }

    re_imgui_draw(&window);

    re_window_end_render_pass(&window);

    re_window_end_frame(&window);
  }

  eg_world_destroy(&world);
  eg_asset_manager_destroy(&asset_manager);

  re_pipeline_destroy(&pbr_pipeline);
  re_pipeline_destroy(&skybox_pipeline);

  eg_engine_destroy();

  re_imgui_destroy();
  re_window_destroy(&window);
  re_context_destroy(&g_ctx);

  return 0;
}
