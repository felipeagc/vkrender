#include <engine/asset_manager.h>
#include <engine/assets/environment_asset.h>
#include <engine/assets/gltf_model_asset.h>
#include <engine/assets/mesh_asset.h>
#include <engine/assets/pipeline_asset.h>
#include <engine/camera.h>
#include <engine/components/gltf_model_component.h>
#include <engine/components/mesh_component.h>
#include <engine/components/transform_component.h>
#include <engine/engine.h>
#include <engine/imgui.h>
#include <engine/inspector.h>
#include <engine/pbr.h>
#include <engine/pipelines.h>
#include <engine/systems/fps_camera_system.h>
#include <engine/systems/picking_system.h>
#include <engine/systems/rendering_system.h>
#include <engine/util.h>
#include <engine/world.h>
#include <fstd_util.h>
#include <renderer/renderer.h>
#include <string.h>

#ifdef _MSC_VER
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

typedef struct game_t {
  re_window_t window;

  eg_asset_manager_t asset_manager;
  eg_world_t world;

  eg_fps_camera_system_t fps_system;
  eg_picking_system_t picking_system;
  eg_inspector_t inspector;
} game_t;

static void game_init(game_t *game, int argc, const char *argv[]) {
  re_context_init();
  re_window_init(&game->window, "Re-write", 1600, 900);
  eg_imgui_init(&game->window);
  eg_engine_init(argv[0]);
  eg_fs_mount("./assets", "/assets");
  eg_fs_mount("../assets", "/assets");
  eg_fs_mount("../../assets", "/assets");
  eg_fs_mount("./shaders/out", "/shaders");
  eg_fs_mount("../shaders/out", "/shaders");
  eg_fs_mount("../../shaders/out", "/shaders");

  game->window.clear_color = (vec4_t){1.0, 1.0, 1.0, 1.0};

  eg_asset_manager_init(&game->asset_manager);

  eg_environment_asset_t *environment_asset = eg_asset_alloc(
      &game->asset_manager, "Environment", eg_environment_asset_t);
  eg_environment_asset_init(
      environment_asset, "/assets/ice_lake.env", "/assets/brdf_lut.png");

  eg_world_init(&game->world, environment_asset);

  // Systems
  uint32_t width, height;
  re_window_get_size(&game->window, &width, &height);
  eg_inspector_init(&game->inspector);
  eg_picking_system_init(
      &game->picking_system,
      &game->window,
      &game->window.render_target,
      &game->world);
  eg_fps_camera_system_init(&game->fps_system);
}

static void game_destroy(game_t *game) {
  eg_world_destroy(&game->world);
  eg_asset_manager_destroy(&game->asset_manager);

  eg_picking_system_destroy(&game->picking_system);

  eg_engine_destroy();

  eg_imgui_destroy();
  re_window_destroy(&game->window);
  re_context_destroy();
}

int main(int argc, const char *argv[]) {
  game_t game;
  game_init(&game, argc, argv);

  eg_pipeline_asset_t *pbr_pipeline =
      eg_asset_alloc(&game.asset_manager, "PBR pipeline", eg_pipeline_asset_t);
  eg_pipeline_asset_init(
      pbr_pipeline,
      &game.window.render_target,
      "/shaders/pbr.vert.spv",
      "/shaders/pbr.frag.spv",
      eg_pbr_pipeline_parameters());

  eg_pipeline_asset_t *skybox_pipeline = eg_asset_alloc(
      &game.asset_manager, "Skybox pipeline", eg_pipeline_asset_t);
  eg_pipeline_asset_init(
      skybox_pipeline,
      &game.window.render_target,
      "/shaders/skybox.vert.spv",
      "/shaders/skybox.frag.spv",
      eg_skybox_pipeline_parameters());

  {
    eg_gltf_model_asset_t *model_asset =
        eg_asset_alloc(&game.asset_manager, "Helmet", eg_gltf_model_asset_t);
    eg_gltf_model_asset_init(model_asset, "/assets/DamagedHelmet.glb", true);

    eg_entity_t ent = eg_world_add_entity(&game.world);

    eg_transform_component_t *transform_comp =
        eg_world_add_comp(&game.world, ent, EG_TRANSFORM_COMPONENT_TYPE);
    eg_transform_component_init(transform_comp);

    eg_gltf_model_component_t *model_comp =
        eg_world_add_comp(&game.world, ent, EG_GLTF_MODEL_COMPONENT_TYPE);
    eg_gltf_model_component_init(model_comp, model_asset);
  }

  {
    eg_gltf_model_asset_t *model_asset = eg_asset_alloc(
        &game.asset_manager, "Water bottle", eg_gltf_model_asset_t);
    eg_gltf_model_asset_init(model_asset, "/assets/WaterBottle.glb", false);

    eg_entity_t ent = eg_world_add_entity(&game.world);

    eg_transform_component_t *transform_comp =
        eg_world_add_comp(&game.world, ent, EG_TRANSFORM_COMPONENT_TYPE);
    eg_transform_component_init(transform_comp);
    transform_comp->position = (vec3_t){2.0, 0.0, 0.0};
    transform_comp->scale = (vec3_t){10.0, 10.0, 10.0};

    eg_gltf_model_component_t *model_comp =
        eg_world_add_comp(&game.world, ent, EG_GLTF_MODEL_COMPONENT_TYPE);
    eg_gltf_model_component_init(model_comp, model_asset);
  }

  {
    eg_gltf_model_asset_t *model_asset =
        eg_asset_alloc(&game.asset_manager, "Boom box", eg_gltf_model_asset_t);
    eg_gltf_model_asset_init(model_asset, "/assets/BoomBox.glb", false);

    eg_entity_t ent = eg_world_add_entity(&game.world);

    eg_transform_component_t *transform_comp =
        eg_world_add_comp(&game.world, ent, EG_TRANSFORM_COMPONENT_TYPE);
    eg_transform_component_init(transform_comp);
    transform_comp->position = (vec3_t){-2.0, 0.0, 0.0};
    transform_comp->scale = (vec3_t){100.0, 100.0, 100.0};

    eg_gltf_model_component_t *model_comp =
        eg_world_add_comp(&game.world, ent, EG_GLTF_MODEL_COMPONENT_TYPE);
    eg_gltf_model_component_init(model_comp, model_asset);
  }

  while (!re_window_should_close(&game.window)) {
    re_window_poll_events(&game.window);

    re_event_t event;
    while (re_window_next_event(&game.window, &event)) {
      eg_imgui_process_event(&event);
      eg_picking_system_process_event(
          &game.picking_system, &event, &game.inspector.selected_entity);
    }

    uint32_t width, height;
    re_window_get_size(&game.window, &width, &height);

    const eg_cmd_info_t cmd_info = {
        .frame_index = game.window.current_frame,
        .cmd_buffer = re_window_get_current_command_buffer(&game.window),
    };

    eg_imgui_begin();
    eg_inspector_draw_ui(
        &game.inspector, &game.window, &game.world, &game.asset_manager);
    eg_imgui_end();

    // Per-frame updates
    eg_environment_update(&game.world.environment, &cmd_info);

    // Begin command buffer recording
    re_window_begin_frame(&game.window);

    eg_fps_camera_system_update(
        &game.fps_system,
        &game.window,
        &game.world.camera,
        &cmd_info,
        (float)width,
        (float)height);

    // Begin window renderpass
    re_window_begin_render_pass(&game.window);

    eg_camera_bind(
        &game.world.camera, &cmd_info, &skybox_pipeline->pipeline, 0);
    eg_environment_draw_skybox(
        &game.world.environment, &cmd_info, &skybox_pipeline->pipeline);

    eg_rendering_system_render(&cmd_info, &game.world, &pbr_pipeline->pipeline);

    eg_picking_system_draw_gizmos(
        &game.picking_system, &cmd_info, game.inspector.selected_entity);

    eg_picking_system_update(
        &game.picking_system, game.inspector.selected_entity);

    eg_imgui_draw(&cmd_info);

    // End window renderpass
    re_window_end_render_pass(&game.window);

    // End command buffer recording
    re_window_end_frame(&game.window);
  }

  game_destroy(&game);

  return 0;
}
