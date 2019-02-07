#include <engine/asset_manager.hpp>
#include <engine/assets/environment_asset.hpp>
#include <engine/camera.hpp>
#include <engine/pipelines.hpp>
#include <engine/systems/fps_camera_system.hpp>
#include <engine/world.hpp>
#include <imgui/imgui.h>
#include <renderer/renderer.hpp>

int main() {
  re_window_t window;
  re_window_init(&window, "Re-write", 1600, 900);

  re_imgui_t imgui;
  re_imgui_init(&imgui, &window);

  window.clear_color = {1.0, 1.0, 1.0, 1.0};

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

    if (ImGui::Begin("Hello!")) {
      float deg = to_degrees(world.camera.fov);
      ImGui::DragFloat("FOV", &deg, 0.1f);
      world.camera.fov = to_radians(deg);
      ImGui::End();
    }

    re_imgui_end(&imgui);

    re_window_begin_frame(&window);

    eg_fps_camera_system_update(&fps_system, &window, &world.camera);

    re_window_begin_render_pass(&window);

    eg_camera_bind(&world.camera, &window, &skybox_pipeline, 0);
    eg_environment_draw_skybox(&world.environment, &window, &skybox_pipeline);

    re_imgui_draw(&imgui);

    re_window_end_render_pass(&window);

    re_window_end_frame(&window);
  }

  eg_world_destroy(&world);
  eg_environment_asset_destroy(environment_asset);
  eg_asset_manager_destroy(&asset_manager);

  re_pipeline_destroy(&skybox_pipeline);

  re_imgui_destroy(&imgui);
  re_window_destroy(&window);
  re_context_destroy(&g_ctx);

  return 0;
}