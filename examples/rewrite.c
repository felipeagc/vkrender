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

static void
game_framebuffer_resize_callback(re_window_t *window, int width, int height) {
  game_t *game = window->user_ptr;

  eg_picking_system_resize(
      &game->picking_system, (uint32_t)width, (uint32_t)height);
}

static void game_mouse_button_callback(
    re_window_t *window, int button, int action, int mods) {
  game_t *game = window->user_ptr;

  eg_imgui_mouse_button_callback(window, button, action, mods);

  double mouse_x, mouse_y;
  re_window_get_cursor_pos(window, &mouse_x, &mouse_y);

  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (action == GLFW_PRESS) {
      eg_picking_system_mouse_press(
          &game->picking_system,
          &game->world,
          &game->inspector.selected_entity,
          window->current_frame,
          (uint32_t)mouse_x,
          (uint32_t)mouse_y);
    } else if (action == GLFW_RELEASE) {
      eg_picking_system_mouse_release(&game->picking_system);
    }
  }
}

static void game_scroll_callback(re_window_t *window, double x, double y) {
  eg_imgui_scroll_callback(window, x, y);
}

static void game_key_callback(
    re_window_t *window, int key, int scancode, int action, int mods) {
  eg_imgui_key_callback(window, key, scancode, action, mods);
}

static void game_char_callback(re_window_t *window, unsigned int c) {
  eg_imgui_char_callback(window, c);
}

static void game_cursor_pos_callback(re_window_t *window, double x, double y) {
  game_t *game = window->user_ptr;

  uint32_t width, height;
  re_window_get_size(window, &width, &height);
}

static void game_init(game_t *game, int argc, const char *argv[]) {
  re_context_init();
  re_window_init(&game->window, "Re-write", 1600, 900);
  eg_imgui_init(&game->window);
  eg_engine_init(argv[0]);
  assert(eg_mount("./assets", "/assets"));
  assert(eg_mount("./shaders/out", "/shaders"));

  game->window.clear_color = (vec4_t){1.0, 1.0, 1.0, 1.0};
  game->window.user_ptr = game;
  game->window.mouse_button_callback = game_mouse_button_callback;
  game->window.scroll_callback = game_scroll_callback;
  game->window.key_callback = game_key_callback;
  game->window.char_callback = game_char_callback;
  game->window.cursor_pos_callback = game_cursor_pos_callback;
  game->window.framebuffer_resize_callback = game_framebuffer_resize_callback;

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
      &game->picking_system, &game->window.render_target, width, height);
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
    uint32_t width, height;
    re_window_get_size(&game.window, &width, &height);

    re_window_poll_events(&game.window);

    /* double mouse_x, mouse_y; */
    /* re_window_get_cursor_pos(&game.window, &mouse_x, &mouse_y); */

    /* eg_picking_system_pick( */
    /*     &game.picking_system, */
    /*     &game.world, */
    /*     game.inspector.selected_entity, */
    /*     game.window.current_frame, */
    /*     (uint32_t)mouse_x, */
    /*     (uint32_t)mouse_y); */

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
        &game.picking_system,
        &game.world,
        game.inspector.selected_entity,
        &cmd_info,
        width,
        height);

    eg_picking_system_update(
        &game.picking_system,
        &game.window,
        &game.world,
        game.inspector.selected_entity,
        width,
        height);

    eg_imgui_draw(&cmd_info);

    // End window renderpass
    re_window_end_render_pass(&game.window);

    // End command buffer recording
    re_window_end_frame(&game.window);
  }

  game_destroy(&game);

  return 0;
}
