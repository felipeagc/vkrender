#include <engine/asset_manager.h>
#include <engine/assets/environment_asset.h>
#include <engine/assets/gltf_model_asset.h>
#include <engine/assets/mesh_asset.h>
#include <engine/camera.h>
#include <engine/components/gltf_model_component.h>
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

typedef struct game_t {
  re_window_t window;

  re_pipeline_t pbr_pipeline;
  re_pipeline_t skybox_pipeline;

  eg_asset_manager_t asset_manager;
  eg_world_t world;

  eg_fps_camera_system_t fps_system;
} game_t;

void game_mouse_button_callback(
    re_window_t *window, int button, int action, int mods) {
  re_imgui_mouse_button_callback(window, button, action, mods);
}

void game_scroll_callback(re_window_t *window, double x, double y) {
  re_imgui_scroll_callback(window, x, y);
}

void game_key_callback(
    re_window_t *window, int key, int scancode, int action, int mods) {
  re_imgui_key_callback(window, key, scancode, action, mods);
}

void game_char_callback(re_window_t *window, unsigned int c) {
  re_imgui_char_callback(window, c);
}

int main(int argc, const char *argv[]) {
  game_t game;

  re_context_init();
  re_window_init(&game.window, "Re-write", 1600, 900);
  re_imgui_init(&game.window);
  eg_engine_init(argv[0]);
  assert(eg_mount("./assets", "/assets"));
  assert(eg_mount("./shaders/out", "/shaders"));

  game.window.clear_color = (vec4_t){1.0, 1.0, 1.0, 1.0};
  game.window.user_ptr = &game;
  game.window.mouse_button_callback = game_mouse_button_callback;
  game.window.scroll_callback = game_scroll_callback;
  game.window.key_callback = game_key_callback;
  game.window.char_callback = game_char_callback;

  eg_init_pipeline_spv(
      &game.pbr_pipeline,
      &game.window.render_target,
      "/shaders/pbr.vert.spv",
      "/shaders/pbr.frag.spv",
      eg_pbr_pipeline_parameters());

  eg_init_pipeline_spv(
      &game.skybox_pipeline,
      &game.window.render_target,
      "/shaders/skybox.vert.spv",
      "/shaders/skybox.frag.spv",
      eg_skybox_pipeline_parameters());

  eg_asset_manager_init(&game.asset_manager);

  eg_environment_asset_t *environment_asset =
      eg_asset_alloc(&game.asset_manager, eg_environment_asset_t);
  eg_environment_asset_init(
      environment_asset, "/assets/ice_lake.env", "/assets/brdf_lut.png");

  eg_gltf_model_asset_t *model_asset =
      eg_asset_alloc(&game.asset_manager, eg_gltf_model_asset_t);
  eg_gltf_model_asset_init(model_asset, "/assets/DamagedHelmet.glb");

  eg_world_init(&game.world, environment_asset);

  eg_fps_camera_system_init(&game.fps_system);

  re_vertex_t vertices[] = {
      {{-1, -1, 0}, {0, 0, 1}, {0, 0}},
      {{1, -1, 0}, {0, 0, 1}, {1, 0}},
      {{1, 1, 0}, {0, 0, 1}, {1, 1}},
      {{-1, 1, 0}, {0, 0, 1}, {0, 1}},
  };

  uint32_t indices[] = {0, 1, 2, 2, 3, 0};

  eg_mesh_asset_t *mesh_asset =
      eg_asset_alloc(&game.asset_manager, eg_mesh_asset_t);
  eg_mesh_asset_init(
      mesh_asset, vertices, ARRAY_SIZE(vertices), indices, ARRAY_SIZE(indices));

  eg_pbr_material_asset_t *material =
      eg_asset_alloc(&game.asset_manager, eg_pbr_material_asset_t);
  eg_pbr_material_asset_init(material, NULL, NULL, NULL, NULL, NULL);

  {
    eg_entity_t ent = eg_world_add_entity(&game.world);

    eg_transform_component_t *transform_comp =
        eg_world_add_comp(&game.world, ent, EG_TRANSFORM_COMPONENT_TYPE);
    eg_transform_component_init(transform_comp);

    /* eg_mesh_component_t *mesh_comp = */
    /*     eg_world_add_comp(&world, ent, EG_MESH_COMPONENT_TYPE); */
    /* eg_mesh_component_init(mesh_comp, mesh_asset, material); */

    eg_gltf_model_component_t *model_comp =
        eg_world_add_comp(&game.world, ent, EG_GLTF_MODEL_COMPONENT_TYPE);
    eg_gltf_model_component_init(model_comp, model_asset);
  }

  while (!re_window_should_close(&game.window)) {
    re_window_poll_events(&game.window);

    // Per-frame updates
    eg_environment_update(&game.world.environment, &game.window);

    re_imgui_begin(&game.window);

    eg_draw_inspector(&game.world, &game.asset_manager);

    re_imgui_end();

    re_window_begin_frame(&game.window);

    eg_fps_camera_system_update(
        &game.fps_system, &game.window, &game.world.camera);

    re_window_begin_render_pass(&game.window);

    eg_camera_bind(&game.world.camera, &game.window, &game.skybox_pipeline, 0);
    eg_environment_draw_skybox(
        &game.world.environment, &game.window, &game.skybox_pipeline);

    re_pipeline_bind_graphics(&game.pbr_pipeline, &game.window);
    eg_camera_bind(&game.world.camera, &game.window, &game.pbr_pipeline, 0);
    eg_environment_bind(
        &game.world.environment, &game.window, &game.pbr_pipeline, 1);

    // Draw all meshes
    for (eg_entity_t entity = 0; entity < EG_MAX_ENTITIES; entity++) {
      if (eg_world_has_comp(&game.world, entity, EG_MESH_COMPONENT_TYPE) &&
          eg_world_has_comp(&game.world, entity, EG_TRANSFORM_COMPONENT_TYPE)) {
        eg_mesh_component_t *mesh =
            EG_GET_COMP(&game.world, entity, eg_mesh_component_t);
        eg_transform_component_t *transform =
            EG_GET_COMP(&game.world, entity, eg_transform_component_t);

        mesh->model.uniform.transform =
            eg_transform_component_to_mat4(transform);
        eg_mesh_component_draw(mesh, &game.window, &game.pbr_pipeline);
      }

      if (eg_world_has_comp(
              &game.world, entity, EG_GLTF_MODEL_COMPONENT_TYPE) &&
          eg_world_has_comp(&game.world, entity, EG_TRANSFORM_COMPONENT_TYPE)) {
        eg_gltf_model_component_t *model =
            EG_GET_COMP(&game.world, entity, eg_gltf_model_component_t);
        eg_transform_component_t *transform =
            EG_GET_COMP(&game.world, entity, eg_transform_component_t);

        eg_gltf_model_component_draw(
            model,
            &game.window,
            &game.pbr_pipeline,
            eg_transform_component_to_mat4(transform));
      }
    }

    re_imgui_draw(&game.window);

    re_window_end_render_pass(&game.window);

    re_window_end_frame(&game.window);
  }

  eg_world_destroy(&game.world);
  eg_asset_manager_destroy(&game.asset_manager);

  re_pipeline_destroy(&game.pbr_pipeline);
  re_pipeline_destroy(&game.skybox_pipeline);

  eg_engine_destroy();

  re_imgui_destroy();
  re_window_destroy(&game.window);
  re_context_destroy();

  return 0;
}
