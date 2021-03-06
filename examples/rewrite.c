#include <engine/all.h>
#include <fstd_util.h>
#include <renderer/renderer.h>

#ifdef _MSC_VER
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

typedef struct game_t {
  re_window_t window;

  eg_asset_manager_t asset_manager;
  eg_scene_t scene;

  eg_fps_camera_system_t fps_system;
  eg_inspector_t inspector;
} game_t;

static eg_entity_t add_gltf(
    game_t *game,
    const char *path,
    eg_pipeline_asset_t *pipeline_asset,
    vec3_t position,
    vec3_t scale,
    bool flip_uvs) {
  eg_gltf_asset_t *model_asset = eg_asset_manager_alloc(
      &game->asset_manager, EG_ASSET_TYPE(eg_gltf_asset_t));
  eg_gltf_asset_init(
      model_asset,
      &(eg_gltf_asset_options_t){.path = path, .flip_uvs = flip_uvs});
  eg_asset_set_name(&model_asset->asset, path);

  eg_entity_t ent = eg_entity_add(&game->scene.entity_manager);

  eg_transform_comp_t *transform =
      EG_ADD_COMP(&game->scene.entity_manager, ent, eg_transform_comp_t);
  transform->position = position;
  transform->scale    = scale;

  eg_gltf_comp_t *model =
      EG_ADD_COMP(&game->scene.entity_manager, ent, eg_gltf_comp_t);
  eg_gltf_comp_init(model, model_asset);

  eg_renderable_comp_t *renderable =
      EG_ADD_COMP(&game->scene.entity_manager, ent, eg_renderable_comp_t);
  eg_renderable_comp_init(renderable, pipeline_asset);

  return ent;
}

static eg_entity_t
add_light(game_t *game, vec3_t position, vec3_t color, float intensity) {
  eg_entity_t ent = eg_entity_add(&game->scene.entity_manager);

  eg_transform_comp_t *transform_comp =
      EG_ADD_COMP(&game->scene.entity_manager, ent, eg_transform_comp_t);
  transform_comp->position = position;

  eg_point_light_comp_t *light_comp =
      EG_ADD_COMP(&game->scene.entity_manager, ent, eg_point_light_comp_t);
  eg_point_light_comp_init(
      light_comp, (vec4_t){.xyz = color, .w = 1.0f}, intensity);

  return ent;
}

static eg_entity_t
add_terrain(game_t *game, uint32_t dim, eg_pipeline_asset_t *pipeline_asset) {
  uint32_t vertex_count = dim * dim;
  re_vertex_t *vertices = calloc(vertex_count, sizeof(re_vertex_t));

  for (uint32_t i = 0; i < dim; i++) {
    for (uint32_t j = 0; j < dim; j++) {
      float x = ((float)i - (float)dim / 2.0f);
      float z = ((float)j - (float)dim / 2.0f);

      vertices[i * dim + j] = (re_vertex_t){
          .pos    = {x, 0.0f, z},
          .normal = {0.0f, 1.0f, 0.0f},
      };
    }
  }

  uint32_t index_count   = (dim - 1) * (dim - 1) * 6;
  uint32_t *indices      = calloc(index_count, sizeof(uint32_t));
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

  eg_mesh_asset_t *mesh_asset = eg_asset_manager_alloc(
      &game->asset_manager, EG_ASSET_TYPE(eg_mesh_asset_t));
  eg_mesh_asset_init(
      mesh_asset,
      &(eg_mesh_asset_options_t){
          .vertices     = vertices,
          .vertex_count = vertex_count,
          .indices      = indices,
          .index_count  = index_count,
      });
  eg_asset_set_name(&mesh_asset->asset, "Terrain mesh");

  free(indices);
  free(vertices);

  eg_pbr_material_asset_t *mat_asset = eg_asset_manager_alloc(
      &game->asset_manager, EG_ASSET_TYPE(eg_pbr_material_asset_t));
  eg_pbr_material_asset_init(mat_asset, &(eg_pbr_material_asset_options_t){0});
  eg_asset_set_name(&mat_asset->asset, "Terrain material");

  mat_asset->uniform.base_color_factor = (vec4_t){0.0f, 0.228f, 0.456f, 1.0f};

  eg_entity_t ent = eg_entity_add(&game->scene.entity_manager);

  eg_transform_comp_t *transform =
      EG_ADD_COMP(&game->scene.entity_manager, ent, eg_transform_comp_t);
  transform->position = (vec3_t){0.0f, -2.0f, 0.0f};

  eg_mesh_comp_t *mesh =
      EG_ADD_COMP(&game->scene.entity_manager, ent, eg_mesh_comp_t);
  eg_mesh_comp_init(mesh, mesh_asset, mat_asset);

  eg_terrain_comp_t *terrain =
      EG_ADD_COMP(&game->scene.entity_manager, ent, eg_terrain_comp_t);

  eg_renderable_comp_t *renderable =
      EG_ADD_COMP(&game->scene.entity_manager, ent, eg_renderable_comp_t);
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
          .title        = "Re-write",
          .width        = 1600,
          .height       = 900,
          .sample_count = VK_SAMPLE_COUNT_1_BIT,
      });
  game.window.clear_color = (vec4_t){1.0, 1.0, 1.0, 1.0};

  eg_imgui_init(&game.window, &game.window.render_target);

  eg_asset_manager_init(&game.asset_manager);

  eg_image_asset_t *skybox = eg_asset_manager_alloc(
      &game.asset_manager, EG_ASSET_TYPE(eg_image_asset_t));
  eg_image_asset_init(
      skybox,
      &(eg_image_asset_options_t){
          .path = "/assets/environments/bridge_skybox.ktx"});
  eg_asset_set_name(&skybox->asset, "Skybox");

  eg_image_asset_t *irradiance = eg_asset_manager_alloc(
      &game.asset_manager, EG_ASSET_TYPE(eg_image_asset_t));
  eg_image_asset_init(
      irradiance,
      &(eg_image_asset_options_t){
          .path = "/assets/environments/bridge_irradiance.ktx"});
  eg_asset_set_name(&irradiance->asset, "Irradiance");

  eg_image_asset_t *radiance = eg_asset_manager_alloc(
      &game.asset_manager, EG_ASSET_TYPE(eg_image_asset_t));
  eg_image_asset_init(
      radiance,
      &(eg_image_asset_options_t){
          .path = "/assets/environments/bridge_radiance.ktx"});
  eg_asset_set_name(&radiance->asset, "Radiance");

  eg_image_asset_t *brdf = eg_asset_manager_alloc(
      &game.asset_manager, EG_ASSET_TYPE(eg_image_asset_t));
  eg_image_asset_init(
      brdf, &(eg_image_asset_options_t){.path = "/assets/brdf_lut.png"});
  eg_asset_set_name(&brdf->asset, "BRDF LuT");

  eg_scene_init(&game.scene, skybox, irradiance, radiance, brdf);

  // Systems
  eg_inspector_init(
      &game.inspector,
      &game.window,
      &game.window.render_target,
      &game.scene,
      &game.asset_manager);
  eg_fps_camera_system_init(&game.fps_system, &game.scene.camera);

  eg_pipeline_asset_t *pbr_pipeline = eg_asset_manager_alloc(
      &game.asset_manager, EG_ASSET_TYPE(eg_pipeline_asset_t));
  eg_pipeline_asset_init(
      pbr_pipeline,
      &(eg_pipeline_asset_options_t){
          .vert_path = "/shaders/pbr.vert.spv",
          .frag_path = "/shaders/pbr.frag.spv",
          .params    = eg_default_pipeline_params(),
      });
  eg_asset_set_name(&pbr_pipeline->asset, "PBR pipeline");

  eg_pipeline_asset_t *terrain_pipeline = eg_asset_manager_alloc(
      &game.asset_manager, EG_ASSET_TYPE(eg_pipeline_asset_t));
  eg_pipeline_asset_init(
      terrain_pipeline,
      &(eg_pipeline_asset_options_t){
          .vert_path = "/shaders/terrain.vert.spv",
          .frag_path = "/shaders/terrain.frag.spv",
          .params    = eg_default_pipeline_params(),
      });
  eg_asset_set_name(&terrain_pipeline->asset, "Terrain pipeline");

  eg_pipeline_asset_t *skybox_pipeline = eg_asset_manager_alloc(
      &game.asset_manager, EG_ASSET_TYPE(eg_pipeline_asset_t));
  eg_pipeline_asset_init(
      skybox_pipeline,
      &(eg_pipeline_asset_options_t){
          .vert_path = "/shaders/skybox.vert.spv",
          .frag_path = "/shaders/skybox.frag.spv",
          .params    = eg_skybox_pipeline_params(),
      });
  eg_asset_set_name(&skybox_pipeline->asset, "Skybox pipeline");

  game.scene.environment.uniform.sun_direction = (vec3_t){1.0f, -1.0f, 1.0f};

  add_gltf(
      &game,
      "/assets/models/DamagedHelmet.glb",
      pbr_pipeline,
      (vec3_t){0.0, 0.0, 0.0},
      (vec3_t){1.0, 1.0, 1.0},
      true);

  add_gltf(
      &game,
      "/assets/models/WaterBottle.glb",
      pbr_pipeline,
      (vec3_t){2.0, 0.0, 0.0},
      (vec3_t){10.0, 10.0, 10.0},
      false);

  add_gltf(
      &game,
      "/assets/models/BoomBox.glb",
      pbr_pipeline,
      (vec3_t){-2.0, 0.0, 0.0},
      (vec3_t){100.0, 100.0, 100.0},
      false);

  add_light(&game, (vec3_t){3.0, 3.0, 3.0}, (vec3_t){1.0, 0.0, 0.0}, 2.0f);
  add_light(&game, (vec3_t){-3.0, 3.0, -3.0}, (vec3_t){0.0, 1.0, 0.0}, 2.0f);

  add_terrain(&game, 256, terrain_pipeline);

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

    re_cmd_buffer_t *cmd_buffer = re_window_get_cmd_buffer(&game.window);

    eg_imgui_begin();
    if (inspector_enabled) {
      eg_inspector_draw_ui(&game.inspector);
    }
    eg_imgui_end();

    // Begin command buffer recording
    re_ctx_begin_frame();
    re_window_begin_frame(&game.window);

    eg_fps_camera_system_update(&game.fps_system, &game.window, cmd_buffer);

    eg_light_system(&game.scene);

    // Begin window renderpass
    re_window_begin_render_pass(&game.window);

    // Draw the skybox
    eg_camera_bind(
        &game.scene.camera, cmd_buffer, &skybox_pipeline->pipeline, 0);
    eg_environment_draw_skybox(
        &game.scene.environment, cmd_buffer, &skybox_pipeline->pipeline);

    // Draw the entities
    eg_rendering_system(&game.scene, cmd_buffer);

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

  eg_scene_destroy(&game.scene);
  eg_asset_manager_destroy(&game.asset_manager);

  eg_fs_destroy();

  eg_imgui_destroy();
  re_window_destroy(&game.window);
  eg_engine_destroy();
  re_ctx_destroy();

  return 0;
}
