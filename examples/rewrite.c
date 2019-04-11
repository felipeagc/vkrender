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
  re_canvas_t picking_canvas;

  re_pipeline_t picking_pipeline;
  re_pipeline_t pbr_pipeline;
  re_pipeline_t skybox_pipeline;

  eg_asset_manager_t asset_manager;
  eg_world_t world;

  eg_fps_camera_system_t fps_system;
  eg_inspector_t inspector;
} game_t;

static void do_picking(game_t *game, uint32_t mouse_x, uint32_t mouse_y) {
  VkCommandBuffer command_buffer;

  VkFence fence;

  // Create fence
  {
    VkFenceCreateInfo fence_create_info = {0};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = NULL;
    fence_create_info.flags = 0;

    VK_CHECK(vkCreateFence(g_ctx.device, &fence_create_info, NULL, &fence));
  }

  // Create and begin command buffer
  {
    VkCommandBufferAllocateInfo allocate_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        NULL,
        g_ctx.graphics_command_pool,     // commandPool
        VK_COMMAND_BUFFER_LEVEL_PRIMARY, // level
        1,                               // commandBufferCount
    };

    VK_CHECK(vkAllocateCommandBuffers(
        g_ctx.device, &allocate_info, &command_buffer));

    VkCommandBufferBeginInfo command_buffer_begin_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        NULL,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, // flags
        NULL,                                        // pInheritanceInfo
    };

    VK_CHECK(vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info));
  }

  re_canvas_begin(&game->picking_canvas, command_buffer);

  eg_camera_bind(
      &game->world.camera,
      &game->window,
      command_buffer,
      &game->picking_pipeline,
      0);

  for (eg_entity_t entity = 0; entity < EG_MAX_ENTITIES; entity++) {
    if (eg_world_has_comp(&game->world, entity, EG_GLTF_MODEL_COMPONENT_TYPE) &&
        eg_world_has_comp(&game->world, entity, EG_TRANSFORM_COMPONENT_TYPE)) {
      eg_gltf_model_component_t *model =
          EG_GET_COMP(&game->world, entity, eg_gltf_model_component_t);
      eg_transform_component_t *transform =
          EG_GET_COMP(&game->world, entity, eg_transform_component_t);

      vkCmdPushConstants(
          command_buffer,
          game->picking_pipeline.layout,
          VK_SHADER_STAGE_ALL_GRAPHICS,
          0,
          sizeof(uint32_t),
          &entity);

      eg_gltf_model_component_draw_picking(
          model,
          &game->window,
          command_buffer,
          &game->picking_pipeline,
          eg_transform_component_to_mat4(transform));
    }
  }

  re_canvas_end(&game->picking_canvas, command_buffer);

  // End and free command buffer
  {
    VK_CHECK(vkEndCommandBuffer(command_buffer));

    VkSubmitInfo submit_info = {
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        NULL,
        0,               // waitSemaphoreCount
        NULL,            // pWaitSemaphores
        NULL,            // pWaitDstStageMask
        1,               // commandBufferCount
        &command_buffer, // pCommandBuffers
        0,               // signalSemaphoreCount
        NULL,            // pSignalSemaphores
    };

    VK_CHECK(vkQueueSubmit(g_ctx.graphics_queue, 1, &submit_info, fence));

    VK_CHECK(vkWaitForFences(g_ctx.device, 1, &fence, VK_TRUE, UINT64_MAX));

    vkFreeCommandBuffers(
        g_ctx.device, g_ctx.graphics_command_pool, 1, &command_buffer);
  }

  // Destroy fence
  vkDestroyFence(g_ctx.device, fence, NULL);

  re_buffer_t staging_buffer;
  re_buffer_init(
      &staging_buffer,
      &(re_buffer_options_t){
          .type = RE_BUFFER_TYPE_TRANSFER,
          .size = sizeof(uint32_t),
      });

  re_image_transfer_to_buffer(
      game->picking_canvas.resources[0].color.image,
      &staging_buffer,
      mouse_x,
      mouse_y,
      1,
      1,
      0,
      0);

  void *mapping;
  re_buffer_map_memory(&staging_buffer, &mapping);

  uint32_t entity;
  memcpy(&entity, mapping, sizeof(uint32_t));

  game->inspector.selected_entity = entity;

  re_buffer_unmap_memory(&staging_buffer);

  re_buffer_destroy(&staging_buffer);
}

static void
game_framebuffer_resize_callback(re_window_t *window, int width, int height) {
  game_t *game = window->user_ptr;

  re_canvas_resize(&game->picking_canvas, (uint32_t)width, (uint32_t)height);
}

static void game_mouse_button_callback(
    re_window_t *window, int button, int action, int mods) {
  game_t *game = window->user_ptr;

  re_imgui_mouse_button_callback(window, button, action, mods);

  double mouse_x, mouse_y;
  re_window_get_cursor_pos(window, &mouse_x, &mouse_y);

  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS &&
      !igIsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
    do_picking(game, (uint32_t)mouse_x, (uint32_t)mouse_y);
  }
}

static void game_scroll_callback(re_window_t *window, double x, double y) {
  re_imgui_scroll_callback(window, x, y);
}

static void game_key_callback(
    re_window_t *window, int key, int scancode, int action, int mods) {
  re_imgui_key_callback(window, key, scancode, action, mods);
}

static void game_char_callback(re_window_t *window, unsigned int c) {
  re_imgui_char_callback(window, c);
}

static void game_init(game_t *game, int argc, const char *argv[]) {
  re_context_init();
  re_window_init(&game->window, "Re-write", 1600, 900);
  re_imgui_init(&game->window);
  eg_engine_init(argv[0]);
  assert(eg_mount("./assets", "/assets"));
  assert(eg_mount("./shaders/out", "/shaders"));

  eg_inspector_init(&game->inspector);

  uint32_t width, height;
  re_window_get_size(&game->window, &width, &height);
  re_canvas_init(&game->picking_canvas, width, height, VK_FORMAT_R32_UINT);
  game->picking_canvas.clear_color = (VkClearColorValue){
      .uint32 = {UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX},
  };

  game->window.clear_color = (vec4_t){1.0, 1.0, 1.0, 1.0};
  game->window.user_ptr = game;
  game->window.mouse_button_callback = game_mouse_button_callback;
  game->window.scroll_callback = game_scroll_callback;
  game->window.key_callback = game_key_callback;
  game->window.char_callback = game_char_callback;
  game->window.framebuffer_resize_callback = game_framebuffer_resize_callback;

  eg_init_pipeline_spv(
      &game->picking_pipeline,
      &game->picking_canvas.render_target,
      "/shaders/picking.vert.spv",
      "/shaders/picking.frag.spv",
      eg_picking_pipeline_parameters());

  eg_init_pipeline_spv(
      &game->pbr_pipeline,
      &game->window.render_target,
      "/shaders/pbr.vert.spv",
      "/shaders/pbr.frag.spv",
      eg_pbr_pipeline_parameters());

  eg_init_pipeline_spv(
      &game->skybox_pipeline,
      &game->window.render_target,
      "/shaders/skybox.vert.spv",
      "/shaders/skybox.frag.spv",
      eg_skybox_pipeline_parameters());

  eg_asset_manager_init(&game->asset_manager);

  eg_environment_asset_t *environment_asset =
      eg_asset_alloc(&game->asset_manager, eg_environment_asset_t);
  eg_environment_asset_init(
      environment_asset, "/assets/ice_lake.env", "/assets/brdf_lut.png");

  eg_world_init(&game->world, environment_asset);

  eg_fps_camera_system_init(&game->fps_system);
}

static void game_destroy(game_t *game) {
  eg_world_destroy(&game->world);
  eg_asset_manager_destroy(&game->asset_manager);

  re_pipeline_destroy(&game->picking_pipeline);
  re_pipeline_destroy(&game->pbr_pipeline);
  re_pipeline_destroy(&game->skybox_pipeline);

  re_canvas_destroy(&game->picking_canvas);

  eg_engine_destroy();

  re_imgui_destroy();
  re_window_destroy(&game->window);
  re_context_destroy();
}

int main(int argc, const char *argv[]) {
  game_t game;
  game_init(&game, argc, argv);

  {
    eg_gltf_model_asset_t *model_asset =
        eg_asset_alloc(&game.asset_manager, eg_gltf_model_asset_t);
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
    eg_gltf_model_asset_t *model_asset =
        eg_asset_alloc(&game.asset_manager, eg_gltf_model_asset_t);
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
        eg_asset_alloc(&game.asset_manager, eg_gltf_model_asset_t);
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

    VkCommandBuffer command_buffer =
        re_window_get_current_command_buffer(&game.window);

    re_imgui_begin(&game.window);
    eg_draw_inspector(
        &game.inspector, &game.window, &game.world, &game.asset_manager);
    re_imgui_end();

    // Per-frame updates
    eg_environment_update(&game.world.environment, &game.window);

    // Begin command buffer recording
    re_window_begin_frame(&game.window);

    eg_fps_camera_system_update(
        &game.fps_system, &game.window, &game.world.camera);

    // Begin window renderpass
    re_window_begin_render_pass(&game.window);

    eg_camera_bind(
        &game.world.camera,
        &game.window,
        command_buffer,
        &game.skybox_pipeline,
        0);
    eg_environment_draw_skybox(
        &game.world.environment, &game.window, &game.skybox_pipeline);

    re_pipeline_bind_graphics(&game.pbr_pipeline, &game.window);
    eg_camera_bind(
        &game.world.camera,
        &game.window,
        command_buffer,
        &game.pbr_pipeline,
        0);
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

    // End window renderpass
    re_window_end_render_pass(&game.window);

    // End command buffer recording
    re_window_end_frame(&game.window);
  }

  game_destroy(&game);

  return 0;
}
