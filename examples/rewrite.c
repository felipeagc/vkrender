#include <engine/all.h>
#include <fstd_util.h>
#include <renderer/renderer.h>

#ifdef _MSC_VER
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

typedef struct game_t {
  re_window_t window;

  eg_asset_manager_t asset_manager;
  eg_world_t world;

  eg_fps_camera_system_t fps_system;
  eg_inspector_t inspector;
} game_t;

static eg_entity_t add_gltf(
    game_t *game,
    const char *name,
    const char *path,
    eg_pipeline_asset_t *pipeline_asset,
    vec3_t position,
    vec3_t scale,
    bool flip_uvs) {
  eg_gltf_asset_t *model_asset =
      eg_asset_alloc(&game->asset_manager, name, eg_gltf_asset_t);
  eg_gltf_asset_init(model_asset, path, flip_uvs);

  eg_entity_t ent = eg_world_add(&game->world);

  eg_transform_comp_t *transform =
      EG_ADD_COMP(&game->world, eg_transform_comp_t, ent);
  transform->position = position;
  transform->scale = scale;

  eg_gltf_comp_t *model = EG_ADD_COMP(&game->world, eg_gltf_comp_t, ent);
  eg_gltf_comp_init(model, model_asset);

  eg_renderable_comp_t *renderable =
      EG_ADD_COMP(&game->world, eg_renderable_comp_t, ent);
  eg_renderable_comp_init(renderable, pipeline_asset);

  return ent;
}

static eg_entity_t
add_light(game_t *game, vec3_t position, vec3_t color, float intensity) {
  eg_entity_t ent = eg_world_add(&game->world);

  eg_transform_comp_t *transform_comp =
      EG_ADD_COMP(&game->world, eg_transform_comp_t, ent);
  transform_comp->position = position;

  eg_point_light_comp_t *light_comp =
      EG_ADD_COMP(&game->world, eg_point_light_comp_t, ent);
  eg_point_light_comp_init(
      light_comp, (vec4_t){.xyz = color, .w = 1.0f}, intensity);

  return ent;
}

static eg_entity_t add_terrain(
    game_t *game,
    const char *name,
    uint32_t dim,
    eg_pipeline_asset_t *pipeline_asset) {
  uint32_t vertex_count = dim * dim;
  re_vertex_t *vertices = calloc(vertex_count, sizeof(re_vertex_t));

  for (uint32_t i = 0; i < dim; i++) {
    for (uint32_t j = 0; j < dim; j++) {
      float x = ((float)i - (float)dim / 2.0f);
      float z = ((float)j - (float)dim / 2.0f);
      vertices[i * dim + j] = (re_vertex_t){
          .pos = {x, 0.0f, z},
          .normal = {0.0f, 1.0f, 0.0f},
      };
    }
  }

  uint32_t index_count = (dim - 1) * (dim - 1) * 6;
  uint32_t *indices = calloc(index_count, sizeof(uint32_t));
  uint32_t current_index = 0;

  for (uint32_t i = 0; i < dim - 1; i++) {
    for (uint32_t j = 0; j < dim - 1; j++) {
      indices[current_index++] = i * dim + j;
      indices[current_index++] = i * dim + (j + 1);
      indices[current_index++] = (i + 1) * dim + j;

      indices[current_index++] = (i + 1) * dim + j;
      indices[current_index++] = i * dim + (j + 1);
      indices[current_index++] = (i + 1) * dim + (j + 1);
    }
  }

  eg_mesh_asset_t *mesh_asset =
      eg_asset_alloc(&game->asset_manager, name, eg_mesh_asset_t);
  eg_mesh_asset_init(mesh_asset, vertices, vertex_count, indices, index_count);

  free(indices);
  free(vertices);

  eg_pbr_material_asset_t *mat_asset = eg_asset_alloc(
      &game->asset_manager, "Terrain material", eg_pbr_material_asset_t);
  eg_pbr_material_asset_init(mat_asset, NULL, NULL, NULL, NULL, NULL);
  mat_asset->uniform.base_color_factor = (vec4_t){0.0f, 0.228f, 0.456f, 1.0f};

  eg_entity_t ent = eg_world_add(&game->world);

  eg_transform_comp_t *transform =
      EG_ADD_COMP(&game->world, eg_transform_comp_t, ent);
  transform->position = (vec3_t){0.0f, -2.0f, 0.0f};

  eg_mesh_comp_t *mesh = EG_ADD_COMP(&game->world, eg_mesh_comp_t, ent);
  eg_mesh_comp_init(mesh, mesh_asset, mat_asset);

  eg_terrain_comp_t *terrain =
      EG_ADD_COMP(&game->world, eg_terrain_comp_t, ent);

  eg_renderable_comp_t *renderable =
      EG_ADD_COMP(&game->world, eg_renderable_comp_t, ent);
  eg_renderable_comp_init(renderable, pipeline_asset);

  return ent;
}

int main(int argc, const char *argv[]) {
  game_t game;

  re_ctx_init();
  eg_engine_init();

  eg_fs_init(argv[0]);
  eg_fs_mount("./assets", "/assets");
  eg_fs_mount("../assets", "/assets");
  eg_fs_mount("../../assets", "/assets");
  eg_fs_mount("./shaders/out", "/shaders");
  eg_fs_mount("../shaders/out", "/shaders");
  eg_fs_mount("../../shaders/out", "/shaders");

  re_window_init(
      &game.window,
      &(re_window_options_t){
          .title = "Re-write",
          .width = 1600,
          .height = 900,
          .sample_count = VK_SAMPLE_COUNT_1_BIT,
      });
  game.window.clear_color = (vec4_t){1.0, 1.0, 1.0, 1.0};

  eg_imgui_init(&game.window, &game.window.render_target);

  eg_asset_manager_init(&game.asset_manager);

  eg_environment_asset_t *environment_asset = eg_asset_alloc(
      &game.asset_manager, "Environment", eg_environment_asset_t);
  eg_environment_asset_init(
      environment_asset,
      "/assets/environments/bridge_skybox.ktx",
      "/assets/environments/bridge_irradiance.ktx",
      "/assets/environments/bridge_radiance.ktx",
      "/assets/brdf_lut.png");

  eg_world_init(&game.world, environment_asset);

  // Systems
  eg_inspector_init(
      &game.inspector,
      &game.window,
      &game.window.render_target,
      &game.world,
      &game.asset_manager);
  eg_fps_camera_system_init(&game.fps_system, &game.world.camera);

  eg_pipeline_asset_t *pbr_pipeline =
      eg_asset_alloc(&game.asset_manager, "PBR pipeline", eg_pipeline_asset_t);
  eg_pipeline_asset_init(
      pbr_pipeline,
      &game.window.render_target,
      (const char *[]){"/shaders/pbr.vert.spv", "/shaders/pbr.frag.spv"},
      2,
      eg_standard_pipeline_parameters());

  eg_pipeline_asset_t *terrain_pipeline = eg_asset_alloc(
      &game.asset_manager, "Terrain pipeline", eg_pipeline_asset_t);
  eg_pipeline_asset_init(
      terrain_pipeline,
      &game.window.render_target,
      (const char *[]){"/shaders/terrain.vert.spv",
                       "/shaders/terrain.frag.spv"},
      2,
      eg_standard_pipeline_parameters());

  eg_pipeline_asset_t *skybox_pipeline = eg_asset_alloc(
      &game.asset_manager, "Skybox pipeline", eg_pipeline_asset_t);
  eg_pipeline_asset_init(
      skybox_pipeline,
      &game.window.render_target,
      (const char *[]){"/shaders/skybox.vert.spv", "/shaders/skybox.frag.spv"},
      2,
      eg_skybox_pipeline_parameters());

  game.world.environment.uniform.sun_direction = (vec3_t){1.0f, -1.0f, 1.0f};

  /* add_gltf( */
  /*     &game, */
  /*     "Helmet", */
  /*     "/assets/models/DamagedHelmet.glb", */
  /*     pbr_pipeline, */
  /*     (vec3_t){0.0, 0.0, 0.0}, */
  /*     (vec3_t){1.0, 1.0, 1.0}, */
  /*     true); */

  /* add_gltf( */
  /*     &game, */
  /*     "Water bottle", */
  /*     "/assets/models/WaterBottle.glb", */
  /*     pbr_pipeline, */
  /*     (vec3_t){2.0, 0.0, 0.0}, */
  /*     (vec3_t){10.0, 10.0, 10.0}, */
  /*     false); */

  /* add_gltf( */
  /*     &game, */
  /*     "Boom box", */
  /*     "/assets/models/BoomBox.glb", */
  /*     pbr_pipeline, */
  /*     (vec3_t){-2.0, 0.0, 0.0}, */
  /*     (vec3_t){100.0, 100.0, 100.0}, */
  /*     false); */

  add_light(&game, (vec3_t){3.0, 3.0, 3.0}, (vec3_t){1.0, 0.0, 0.0}, 2.0f);
  add_light(&game, (vec3_t){-3.0, 3.0, -3.0}, (vec3_t){0.0, 1.0, 0.0}, 2.0f);

  add_terrain(&game, "Terrain", 256, terrain_pipeline);

  bool inspector_enabled = true;

  while (!re_window_should_close(&game.window)) {
    re_window_poll_events(&game.window);

    re_event_t event;
    while (re_window_next_event(&game.window, &event)) {
      if (event.type == RE_EVENT_KEY_PRESSED) {
        if (event.keyboard.key == GLFW_KEY_I &&
            event.keyboard.mods == GLFW_MOD_CONTROL) {
          inspector_enabled = !inspector_enabled;
        }
      }

      eg_imgui_process_event(&event);

      if (inspector_enabled) {
        eg_inspector_process_event(&game.inspector, &event);
      }
    }

    re_cmd_buffer_t *cmd_buffer =
        re_window_get_current_command_buffer(&game.window);

    eg_world_update(&game.world);

    eg_imgui_begin();
    if (inspector_enabled) {
      eg_inspector_draw_ui(&game.inspector);
    }
    eg_imgui_end();

    // Begin command buffer recording
    re_ctx_begin_frame();
    re_window_begin_frame(&game.window);

    eg_fps_camera_system_update(&game.fps_system, &game.window, cmd_buffer);

    eg_light_system(&game.world);

    // Begin window renderpass
    re_window_begin_render_pass(&game.window);

    // Draw the skybox
    eg_camera_bind(
        &game.world.camera, cmd_buffer, &skybox_pipeline->pipeline, 0);
    eg_environment_draw_skybox(
        &game.world.environment, cmd_buffer, &skybox_pipeline->pipeline);

    // Draw the entities
    eg_rendering_system(&game.world, cmd_buffer);

    if (inspector_enabled) {
      // Draw the selected entity
      eg_inspector_draw_selected_outline(&game.inspector, cmd_buffer);

      // Draw the gizmos
      eg_inspector_draw_gizmos(&game.inspector, cmd_buffer);

      // Update the selected entity's position based on gizmo movement
      eg_inspector_update(&game.inspector);
    }

    // Draw imgui
    eg_imgui_draw(cmd_buffer);

    // End window renderpass
    re_window_end_render_pass(&game.window);

    // End command buffer recording
    re_window_end_frame(&game.window);
  }

  eg_inspector_destroy(&game.inspector);

  eg_world_destroy(&game.world);
  eg_asset_manager_destroy(&game.asset_manager);

  eg_fs_destroy();

  eg_imgui_destroy();
  re_window_destroy(&game.window);
  eg_engine_destroy();
  re_ctx_destroy();

  return 0;
}
