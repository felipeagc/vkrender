#include <engine/asset_manager.hpp>
#include <engine/assets/environment_asset.hpp>
#include <engine/assets/mesh_asset.hpp>
#include <engine/camera.hpp>
#include <engine/components/mesh_component.hpp>
#include <engine/pbr.hpp>
#include <engine/pipelines.hpp>
#include <engine/systems/fps_camera_system.hpp>
#include <engine/world.hpp>
#include <imgui/imgui.h>
#include <renderer/renderer.hpp>
#include <util/array.h>
#include <util/log.h>

int main() {
  re_window_t window;
  re_window_init(&window, "Re-write", 1600, 900);
  re_shader_init_compiler();

  re_imgui_t imgui;
  re_imgui_init(&imgui, &window);

  window.clear_color = {1.0, 1.0, 1.0, 1.0};

  re_pipeline_t pbr_pipeline;
  eg_init_pipeline(
      &pbr_pipeline,
      window.render_target,
      "../shaders/pbr.vert",
      "../shaders/pbr.frag",
      eg_standard_pipeline_parameters());

  re_pipeline_t skybox_pipeline;
  eg_init_pipeline(
      &skybox_pipeline,
      window.render_target,
      "../shaders/skybox.vert",
      "../shaders/skybox.frag",
      eg_skybox_pipeline_parameters());

  eg_asset_manager_t asset_manager;
  eg_asset_manager_init(&asset_manager);

  const char *radiance_paths[] = {
      "../assets/ice_lake/radiance_0_1600x800.hdr",
      "../assets/ice_lake/radiance_1_800x400.hdr",
      "../assets/ice_lake/radiance_2_400x200.hdr",
      "../assets/ice_lake/radiance_3_200x100.hdr",
      "../assets/ice_lake/radiance_4_100x50.hdr",
      "../assets/ice_lake/radiance_5_50x25.hdr",
      "../assets/ice_lake/radiance_6_25x12.hdr",
      "../assets/ice_lake/radiance_7_12x6.hdr",
      "../assets/ice_lake/radiance_8_6x3.hdr",
  };

  eg_environment_asset_t *environment_asset =
      eg_asset_alloc(&asset_manager, eg_environment_asset_t);
  eg_environment_asset_init(
      environment_asset,
      1024,
      1024,
      "../assets/ice_lake/skybox.hdr",
      "../assets/ice_lake/irradiance.hdr",
      ARRAYSIZE(radiance_paths),
      radiance_paths,
      "../assets/brdf_lut.png");

  eg_world_t world;
  eg_world_init(&world, environment_asset);

  eg_fps_camera_system_t fps_system;
  eg_fps_camera_system_init(&fps_system);

  re_vertex_t vertices[] = {
      {{-1, -1, 0}, {0, 0, 0}, {0, 0}},
      {{1, -1, 0}, {0, 0, 0}, {1, 0}},
      {{1, 1, 0}, {0, 0, 0}, {1, 1}},
      {{-1, 1, 0}, {0, 0, 0}, {0, 1}},
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
    eg_mesh_component_t *mesh_comp = (eg_mesh_component_t *)eg_world_add_comp(
        &world, ent, EG_MESH_COMPONENT_TYPE);
    eg_mesh_component_init(mesh_comp, mesh_asset, material);
  }

  {
    eg_entity_t ent = eg_world_add_entity(&world);
    eg_mesh_component_t *mesh_comp = (eg_mesh_component_t *)eg_world_add_comp(
        &world, ent, EG_MESH_COMPONENT_TYPE);
    eg_mesh_component_init(mesh_comp, mesh_asset, material);
  }

  while (!window.should_close) {
    SDL_Event event;
    while (re_window_poll_event(&window, &event)) {
      re_imgui_process_event(&imgui, &event);
      eg_fps_camera_system_process_event(&fps_system, &window, &event);

      switch (event.type) {
      case SDL_QUIT:
        window.should_close = true;
        break;
      }
    }

    // Per-frame updates
    eg_environment_update(&world.environment, &window);

    re_imgui_begin(&imgui);

    if (ImGui::Begin("Camera")) {
      float deg = to_degrees(world.camera.fov);
      ImGui::DragFloat("FOV", &deg, 0.1f);
      world.camera.fov = to_radians(deg);
      ImGui::End();
    }

    if (ImGui::Begin("Meshes")) {
      for (eg_entity_t entity = 0; entity < EG_MAX_ENTITIES; entity++) {
        if (eg_world_has_comp(&world, entity, EG_MESH_COMPONENT_TYPE)) {
          eg_mesh_component_t *mesh =
              EG_GET_COMP(&world, entity, eg_mesh_component_t);

          ImGui::PushID(entity);

          ImGui::Text("Entity: %d", entity);
          ImGui::DragFloat3(
              "Position", mesh->model.uniform.transform.columns[3], 0.1f);
          ImGui::DragFloat3(
              "Local Position",
              mesh->local_model.uniform.transform.columns[3],
              0.1f);
          ImGui::Separator();

          ImGui::PopID();
        }
      }

      ImGui::End();
    }

    re_imgui_end(&imgui);

    re_window_begin_frame(&window);

    eg_fps_camera_system_update(&fps_system, &window, &world.camera);

    re_window_begin_render_pass(&window);

    eg_camera_bind(&world.camera, &window, &skybox_pipeline, 0);
    eg_environment_draw_skybox(&world.environment, &window, &skybox_pipeline);

    re_pipeline_bind_graphics(&pbr_pipeline, &window);
    eg_camera_bind(&world.camera, &window, &pbr_pipeline, 0);
    eg_environment_bind(&world.environment, &window, &pbr_pipeline, 4);

    // Draw all meshes
    for (eg_entity_t entity = 0; entity < EG_MAX_ENTITIES; entity++) {
      if (eg_world_has_comp(&world, entity, EG_MESH_COMPONENT_TYPE)) {
        eg_mesh_component_t *mesh =
            EG_GET_COMP(&world, entity, eg_mesh_component_t);

        eg_mesh_component_draw(mesh, &window, &pbr_pipeline);
      }
    }

    re_imgui_draw(&imgui);

    re_window_end_render_pass(&window);

    re_window_end_frame(&window);
  }

  eg_world_destroy(&world);
  eg_asset_manager_destroy(&asset_manager);

  re_pipeline_destroy(&pbr_pipeline);
  re_pipeline_destroy(&skybox_pipeline);

  re_shader_destroy_compiler();
  re_imgui_destroy(&imgui);
  re_window_destroy(&window);
  re_context_destroy(&g_ctx);

  return 0;
}
