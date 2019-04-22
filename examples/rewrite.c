#include <engine/asset_manager.h>
#include <engine/assets/environment_asset.h>
#include <engine/assets/gltf_model_asset.h>
#include <engine/assets/mesh_asset.h>
#include <engine/assets/pipeline_asset.h>
#include <engine/camera.h>
#include <engine/comps/gltf_model_comp.h>
#include <engine/comps/mesh_comp.h>
#include <engine/comps/point_light_comp.h>
#include <engine/comps/transform_comp.h>
#include <engine/filesystem.h>
#include <engine/imgui.h>
#include <engine/inspector.h>
#include <engine/pbr.h>
#include <engine/pipelines.h>
#include <engine/systems/fps_camera_system.h>
#include <engine/systems/light_system.h>
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

  re_canvas_t scene_canvas;

  eg_fps_camera_system_t fps_system;
  eg_inspector_t inspector;
} game_t;

static void game_init(game_t *game, int argc, const char *argv[]) {
  re_context_init();
  re_window_init(&game->window, "Re-write", 1600, 900);
  eg_imgui_init(&game->window);

  eg_fs_init(argv[0]);
  eg_fs_mount("./assets", "/assets");
  eg_fs_mount("../assets", "/assets");
  eg_fs_mount("../../assets", "/assets");
  eg_fs_mount("./shaders/out", "/shaders");
  eg_fs_mount("../shaders/out", "/shaders");
  eg_fs_mount("../../shaders/out", "/shaders");

  eg_default_pipeline_layouts_init();

  game->window.clear_color = (vec4_t){1.0, 1.0, 1.0, 1.0};

  uint32_t width, height;
  re_window_size(&game->window, &width, &height);
  re_canvas_init(
      &game->scene_canvas,
      &(re_canvas_options_t){
          .width = width,
          .height = height,
          .sample_count = VK_SAMPLE_COUNT_2_BIT,
      });

  eg_asset_manager_init(&game->asset_manager);

  eg_environment_asset_t *environment_asset = eg_asset_alloc(
      &game->asset_manager, "Environment", eg_environment_asset_t);
  eg_environment_asset_init(
      environment_asset, "/assets/ice_lake.env", "/assets/brdf_lut.png");

  eg_world_init(&game->world, environment_asset);

  // Systems
  eg_inspector_init(
      &game->inspector,
      &game->window,
      &game->scene_canvas.render_target,
      &game->world,
      &game->asset_manager);
  eg_fps_camera_system_init(&game->fps_system);
}

static void game_destroy(game_t *game) {
  eg_inspector_destroy(&game->inspector);

  eg_world_destroy(&game->world);
  eg_asset_manager_destroy(&game->asset_manager);

  re_canvas_destroy(&game->scene_canvas);

  eg_default_pipeline_layouts_destroy();

  eg_fs_destroy();

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
      &game.scene_canvas.render_target,
      "/shaders/pbr.vert.spv",
      "/shaders/pbr.frag.spv",
      eg_pbr_pipeline_parameters());

  eg_pipeline_asset_t *skybox_pipeline = eg_asset_alloc(
      &game.asset_manager, "Skybox pipeline", eg_pipeline_asset_t);
  eg_pipeline_asset_init(
      skybox_pipeline,
      &game.scene_canvas.render_target,
      "/shaders/skybox.vert.spv",
      "/shaders/skybox.frag.spv",
      eg_skybox_pipeline_parameters());

  eg_pipeline_asset_t *fullscreen_pipeline = eg_asset_alloc(
      &game.asset_manager, "Fullscreen pipeline", eg_pipeline_asset_t);
  eg_pipeline_asset_init(
      fullscreen_pipeline,
      &game.window.render_target,
      "/shaders/fullscreen.vert.spv",
      "/shaders/fullscreen.frag.spv",
      eg_fullscreen_pipeline_parameters());

  {
    eg_gltf_model_asset_t *model_asset =
        eg_asset_alloc(&game.asset_manager, "Helmet", eg_gltf_model_asset_t);
    eg_gltf_model_asset_init(model_asset, "/assets/DamagedHelmet.glb", true);

    eg_entity_t ent = eg_world_add(&game.world);

    eg_transform_comp_t *transform_comp =
        EG_ADD_COMP(&game.world, eg_transform_comp_t, ent);
    eg_transform_comp_init(transform_comp);

    eg_gltf_model_comp_t *model_comp =
        EG_ADD_COMP(&game.world, eg_gltf_model_comp_t, ent);
    eg_gltf_model_comp_init(model_comp, model_asset);
  }

  {
    eg_gltf_model_asset_t *model_asset = eg_asset_alloc(
        &game.asset_manager, "Water bottle", eg_gltf_model_asset_t);
    eg_gltf_model_asset_init(model_asset, "/assets/WaterBottle.glb", false);

    eg_entity_t ent = eg_world_add(&game.world);

    eg_transform_comp_t *transform_comp =
        EG_ADD_COMP(&game.world, eg_transform_comp_t, ent);
    eg_transform_comp_init(transform_comp);
    transform_comp->position = (vec3_t){2.0, 0.0, 0.0};
    transform_comp->scale = (vec3_t){10.0, 10.0, 10.0};

    eg_gltf_model_comp_t *model_comp =
        EG_ADD_COMP(&game.world, eg_gltf_model_comp_t, ent);
    eg_gltf_model_comp_init(model_comp, model_asset);
  }

  {
    eg_gltf_model_asset_t *model_asset =
        eg_asset_alloc(&game.asset_manager, "Boom box", eg_gltf_model_asset_t);
    eg_gltf_model_asset_init(model_asset, "/assets/BoomBox.glb", false);

    eg_entity_t ent = eg_world_add(&game.world);

    eg_transform_comp_t *transform_comp =
        EG_ADD_COMP(&game.world, eg_transform_comp_t, ent);
    eg_transform_comp_init(transform_comp);
    transform_comp->position = (vec3_t){-2.0, 0.0, 0.0};
    transform_comp->scale = (vec3_t){100.0, 100.0, 100.0};

    eg_gltf_model_comp_t *model_comp =
        EG_ADD_COMP(&game.world, eg_gltf_model_comp_t, ent);
    eg_gltf_model_comp_init(model_comp, model_asset);
  }

  {
    eg_entity_t ent = eg_world_add(&game.world);

    eg_transform_comp_t *transform_comp =
        EG_ADD_COMP(&game.world, eg_transform_comp_t, ent);
    eg_transform_comp_init(transform_comp);
    transform_comp->position = (vec3_t){0.0, 0.0, 0.0};

    eg_point_light_comp_t *light_comp =
        EG_ADD_COMP(&game.world, eg_point_light_comp_t, ent);
    eg_point_light_comp_init(light_comp, (vec4_t){1.0, 0.0, 0.0, 1.0});
  }

  while (!re_window_should_close(&game.window)) {
    re_window_poll_events(&game.window);

    re_event_t event;
    while (re_window_next_event(&game.window, &event)) {
      eg_imgui_process_event(&event);
      eg_inspector_process_event(&game.inspector, &event);

      if (event.type == RE_EVENT_FRAMEBUFFER_RESIZED) {
        re_canvas_resize(
            &game.scene_canvas,
            (uint32_t)event.size.width,
            (uint32_t)event.size.height);
      }
    }

    uint32_t width, height;
    re_window_size(&game.window, &width, &height);

    const eg_cmd_info_t cmd_info = {
        .frame_index = game.window.current_frame,
        .cmd_buffer = re_window_get_current_command_buffer(&game.window),
    };

    eg_world_update(&game.world);

    eg_imgui_begin();
    eg_inspector_draw_ui(&game.inspector);
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

    eg_light_system(&game.world);

    re_canvas_begin(&game.scene_canvas, cmd_info.cmd_buffer);

    // Draw the skybox
    eg_camera_bind(
        &game.world.camera, &cmd_info, &skybox_pipeline->pipeline, 0);
    eg_environment_draw_skybox(
        &game.world.environment, &cmd_info, &skybox_pipeline->pipeline);

    // Draw the entities
    eg_rendering_system(&game.world, &cmd_info, &pbr_pipeline->pipeline);

    // Draw the selected entity
    eg_inspector_draw_selected_outline(&game.inspector, &cmd_info);

    // Draw the gizmos
    eg_inspector_draw_gizmos(&game.inspector, &cmd_info);

    // Update the selected entity's position based on gizmo movement
    eg_inspector_update(&game.inspector);

    re_canvas_end(&game.scene_canvas, cmd_info.cmd_buffer);

    // Begin window renderpass
    re_window_begin_render_pass(&game.window);

    // Draw the scene canvas
    re_canvas_draw(
        &game.scene_canvas,
        cmd_info.cmd_buffer,
        &fullscreen_pipeline->pipeline);

    // Draw imgui
    eg_imgui_draw(&cmd_info);

    // End window renderpass
    re_window_end_render_pass(&game.window);

    // End command buffer recording
    re_window_end_frame(&game.window);
  }

  game_destroy(&game);

  return 0;
}
