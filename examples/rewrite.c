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
  eg_gltf_model_asset_t *model_asset =
      eg_asset_alloc(&game->asset_manager, name, eg_gltf_model_asset_t);
  eg_gltf_model_asset_init(model_asset, path, flip_uvs);

  eg_entity_t ent = eg_world_add(&game->world);

  eg_transform_comp_t *transform =
      EG_ADD_COMP(&game->world, eg_transform_comp_t, ent);
  eg_transform_comp_init(transform);
  transform->position = position;
  transform->scale = scale;

  eg_gltf_model_comp_t *model =
      EG_ADD_COMP(&game->world, eg_gltf_model_comp_t, ent);
  eg_gltf_model_comp_init(model, model_asset);

  eg_renderable_comp_t *renderable =
      EG_ADD_COMP(&game->world, eg_renderable_comp_t, ent);
  eg_renderable_comp_init(renderable, pipeline_asset);

  return ent;
}

static eg_entity_t add_light(game_t *game, vec3_t position, vec3_t color) {
  eg_entity_t ent = eg_world_add(&game->world);

  eg_transform_comp_t *transform_comp =
      EG_ADD_COMP(&game->world, eg_transform_comp_t, ent);
  eg_transform_comp_init(transform_comp);
  transform_comp->position = position;

  eg_point_light_comp_t *light_comp =
      EG_ADD_COMP(&game->world, eg_point_light_comp_t, ent);
  eg_point_light_comp_init(light_comp, (vec4_t){.xyz = color, .w = 1.0f});

  return ent;
}

static eg_entity_t add_heightmap(
    game_t *game,
    const char *name,
    uint32_t dim,
    eg_pipeline_asset_t *pipeline_asset) {
  eg_mesh_asset_t *mesh_asset =
      eg_asset_alloc(&game->asset_manager, name, eg_mesh_asset_t);

  const float terrain_scale = 0.02f;

  re_vertex_t vertices[dim * dim];

  srand(time(0));

  for (uint32_t i = 0; i < dim; i++) {
    for (uint32_t j = 0; j < dim; j++) {
      float x = ((float)i - (float)dim / 2.0f) * terrain_scale;
      float z = ((float)j - (float)dim / 2.0f) * terrain_scale;
      float y = ((float)rand() / (float)(RAND_MAX)) * 2.0f - 1.0f;
      vertices[i * dim + j] = (re_vertex_t){
          .pos = {x, y, z},
          .normal = {0.0f, 1.0f, 0.0f},
      };
    }
  }

  for (uint32_t i = 1; i < dim - 1; i++) {
    for (uint32_t j = 1; j < dim - 1; j++) {
      re_vertex_t left = vertices[(i - 1) * dim + j];
      re_vertex_t right = vertices[(i + 1) * dim + j];
      re_vertex_t bottom = vertices[i * dim + (j + 1)];
      re_vertex_t top = vertices[i * dim + (j - 1)];

      vertices[i * dim + j].normal = vec3_normalize((vec3_t){
          (left.pos.y - right.pos.y),
          terrain_scale * 2.0f,
          (top.pos.y - bottom.pos.y),
      });

      vec3_t normal = vertices[i * dim + j].normal;
    }
  }

  uint32_t indices[(dim - 1) * (dim - 1) * 6];
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

  eg_mesh_asset_init(
      mesh_asset, vertices, ARRAY_SIZE(vertices), indices, ARRAY_SIZE(indices));

  eg_pbr_material_asset_t *mat_asset = eg_asset_alloc(
      &game->asset_manager, "Terrain material", eg_pbr_material_asset_t);
  eg_pbr_material_asset_init(mat_asset, NULL, NULL, NULL, NULL, NULL);
  mat_asset->uniform.base_color_factor = (vec4_t){0.0f, 0.228f, 0.456f, 1.0f};

  eg_entity_t ent = eg_world_add(&game->world);

  eg_transform_comp_t *transform =
      EG_ADD_COMP(&game->world, eg_transform_comp_t, ent);
  eg_transform_comp_init(transform);
  transform->scale = (vec3_t){100.0f, 1.0f, 100.0f};
  transform->position = (vec3_t){0.0f, -2.0f, 0.0f};

  eg_mesh_comp_t *mesh = EG_ADD_COMP(&game->world, eg_mesh_comp_t, ent);
  eg_mesh_comp_init(mesh, mesh_asset, mat_asset);

  eg_renderable_comp_t *renderable =
      EG_ADD_COMP(&game->world, eg_renderable_comp_t, ent);
  eg_renderable_comp_init(renderable, pipeline_asset);

  return ent;
}

int main(int argc, const char *argv[]) {
  game_t game;

  re_context_init();
  eg_engine_init();
  re_window_init(
      &game.window,
      &(re_window_options_t){
          .title = "Re-write",
          .width = 1600,
          .height = 900,
          .sample_count = VK_SAMPLE_COUNT_1_BIT,
      });
  eg_imgui_init(&game.window, &game.window.render_target);

  eg_fs_init(argv[0]);
  eg_fs_mount("./assets", "/assets");
  eg_fs_mount("../assets", "/assets");
  eg_fs_mount("../../assets", "/assets");
  eg_fs_mount("./shaders/out", "/shaders");
  eg_fs_mount("../shaders/out", "/shaders");
  eg_fs_mount("../../shaders/out", "/shaders");

  eg_default_pipeline_layouts_init();

  game.window.clear_color = (vec4_t){1.0, 1.0, 1.0, 1.0};

  eg_asset_manager_init(&game.asset_manager);

  eg_environment_asset_t *environment_asset = eg_asset_alloc(
      &game.asset_manager, "Environment", eg_environment_asset_t);
  eg_environment_asset_init(
      environment_asset, "/assets/ice_lake.env", "/assets/brdf_lut.png");

  eg_world_init(&game.world, environment_asset);

  // Systems
  eg_inspector_init(
      &game.inspector,
      &game.window,
      &game.window.render_target,
      &game.world,
      &game.asset_manager);
  eg_fps_camera_system_init(&game.fps_system);

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

  game.world.environment.uniform.sun_direction = (vec3_t){-1.0f, -0.3f, 1.0f};

  add_gltf(
      &game,
      "Helmet",
      "/assets/DamagedHelmet.glb",
      pbr_pipeline,
      (vec3_t){0.0, 0.0, 0.0},
      (vec3_t){1.0, 1.0, 1.0},
      true);

  add_gltf(
      &game,
      "Water bottle",
      "/assets/WaterBottle.glb",
      pbr_pipeline,
      (vec3_t){2.0, 0.0, 0.0},
      (vec3_t){10.0, 10.0, 10.0},
      false);

  add_gltf(
      &game,
      "Boom box",
      "/assets/BoomBox.glb",
      pbr_pipeline,
      (vec3_t){-2.0, 0.0, 0.0},
      (vec3_t){100.0, 100.0, 100.0},
      false);

  add_light(&game, (vec3_t){3.0, 3.0, 3.0}, (vec3_t){1.0, 0.0, 0.0});
  add_light(&game, (vec3_t){-3.0, 3.0, -3.0}, (vec3_t){0.0, 1.0, 0.0});

  add_heightmap(&game, "Terrain", 50, pbr_pipeline);

  while (!re_window_should_close(&game.window)) {
    re_window_poll_events(&game.window);

    re_event_t event;
    while (re_window_next_event(&game.window, &event)) {
      eg_imgui_process_event(&event);
      eg_inspector_process_event(&game.inspector, &event);
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

    // Begin window renderpass
    re_window_begin_render_pass(&game.window);

    // Draw the skybox
    eg_camera_bind(
        &game.world.camera, &cmd_info, &skybox_pipeline->pipeline, 0);
    eg_environment_draw_skybox(
        &game.world.environment, &cmd_info, &skybox_pipeline->pipeline);

    // Draw the entities
    eg_rendering_system(&game.world, &cmd_info);

    // Draw the selected entity
    eg_inspector_draw_selected_outline(&game.inspector, &cmd_info);

    // Draw the gizmos
    eg_inspector_draw_gizmos(&game.inspector, &cmd_info);

    // Update the selected entity's position based on gizmo movement
    eg_inspector_update(&game.inspector);

    // Draw imgui
    eg_imgui_draw(&cmd_info);

    // End window renderpass
    re_window_end_render_pass(&game.window);

    // End command buffer recording
    re_window_end_frame(&game.window);
  }

  eg_inspector_destroy(&game.inspector);

  eg_world_destroy(&game.world);
  eg_asset_manager_destroy(&game.asset_manager);

  eg_default_pipeline_layouts_destroy();

  eg_fs_destroy();

  eg_imgui_destroy();
  re_window_destroy(&game.window);
  eg_engine_destroy();
  re_context_destroy();

  return 0;
}
