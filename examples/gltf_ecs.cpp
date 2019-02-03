#include <ecs/world.hpp>
#include <engine/engine.hpp>
#include <ftl/logging.hpp>
#include <ftl/vector.hpp>
#include <imgui/imgui.h>
#include <renderer/renderer.hpp>
#include <util/file.hpp>

int main() {
  re_window_t window;
  re_window_init(&window, "GLTF models", 1600, 900);

  re_imgui_t imgui;
  re_imgui_init(&imgui, &window);

  window.clear_color = {0.15, 0.15, 0.15, 1.0};

  // @NOTE: Using this scope to destroy stuff in order for now
  {
    engine::Scene scene("../assets/main.sdf");
    auto &world = scene.m_world;
    auto &assetManager = scene.m_assetManager;

    engine::GltfModelSystem gltfModelSystem;
    engine::FPSCameraSystem fpsCameraSystem;
    engine::BillboardSystem billboardSystem;
    engine::SkyboxSystem skyboxSystem;
    engine::LightingSystem lightingSystem;
    engine::EntityInspectorSystem entityInspectorSystem{assetManager};

    uint32_t width, height;
    re_window_get_size(&window, &width, &height);

    re_canvas_t canvas;
    re_canvas_init(&canvas, width, height);

    // Create shaders & pipelines
    re_pipeline_t model_pipeline;
    eg_init_pipeline(
        &model_pipeline,
        canvas.render_target,
        "../shaders/model_pbr.vert",
        "../shaders/model_pbr.frag",
        eg_standard_pipeline_parameters());

    re_pipeline_t billboard_pipeline;
    eg_init_pipeline(
        &billboard_pipeline,
        canvas.render_target,
        "../shaders/billboard.vert",
        "../shaders/billboard.frag",
        eg_billboard_pipeline_parameters());

    re_pipeline_t wireframe_pipeline;
    eg_init_pipeline(
        &wireframe_pipeline,
        canvas.render_target,
        "../shaders/box.vert",
        "../shaders/box.frag",
        eg_wireframe_pipeline_parameters());

    re_pipeline_t skybox_pipeline;
    eg_init_pipeline(
        &skybox_pipeline,
        canvas.render_target,
        "../shaders/skybox.vert",
        "../shaders/skybox.frag",
        eg_skybox_pipeline_parameters());

    re_pipeline_t fullscreen_pipeline;
    eg_init_pipeline(
        &fullscreen_pipeline,
        window.render_target,
        "../shaders/fullscreen.vert",
        "../shaders/fullscreen.frag",
        eg_fullscreen_pipeline_parameters());

    float time = 0.0;

    bool drawImgui = false;

    while (!window.should_close) {
      time += window.delta_time;

      SDL_Event event;
      while (re_window_poll_event(&window, &event)) {
        re_imgui_process_event(&imgui, &event);
        fpsCameraSystem.processEvent(&window, event);
        entityInspectorSystem.processEvent(&window, assetManager, world, event);

        switch (event.type) {
        case SDL_KEYDOWN:
          if (event.key.keysym.scancode ==
                  (SDL_Scancode)renderer::Scancode::eEscape &&
              !event.key.repeat) {
            drawImgui = !drawImgui;
          }
          break;
        case SDL_WINDOWEVENT:
          if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
            re_canvas_resize(
                &canvas,
                (uint32_t)event.window.data1,
                (uint32_t)event.window.data2);
          }
          break;
        case SDL_QUIT:
          ftl::info("Goodbye");
          window.should_close = true;
          break;
        }
      }

      re_window_begin_frame(&window);

      auto command_buffer = re_window_get_current_command_buffer(&window);

      re_imgui_begin(&imgui);

      if (drawImgui) {
        engine::imgui::statsWindow(&window);
        engine::imgui::assetsWindow(assetManager);
        entityInspectorSystem.imgui(world);
      }

      lightingSystem.process(&window, world);
      fpsCameraSystem.process(&window, world);

      // modelShaderWatcher.lockPipeline();

      // Render target pass
      {
        re_canvas_begin(&canvas, command_buffer);

        entityInspectorSystem.drawBox(
            &window, assetManager, world, wireframe_pipeline);
        gltfModelSystem.process(&window, assetManager, world, model_pipeline);
        skyboxSystem.process(&window, world, skybox_pipeline);
        billboardSystem.process(&window, world, billboard_pipeline);

        re_canvas_end(&canvas, command_buffer);
      }

      re_imgui_end(&imgui);

      // Window pass
      {
        re_window_begin_render_pass(&window);

        re_canvas_draw(&canvas, command_buffer, &fullscreen_pipeline);

        if (drawImgui) {
          re_imgui_draw(&imgui);
        }

        re_window_end_render_pass(&window);
      }

      re_window_end_frame(&window);
    }

    re_pipeline_destroy(&model_pipeline);
    re_pipeline_destroy(&billboard_pipeline);
    re_pipeline_destroy(&wireframe_pipeline);
    re_pipeline_destroy(&skybox_pipeline);
    re_pipeline_destroy(&fullscreen_pipeline);

    re_canvas_destroy(&canvas);
  }

  re_imgui_destroy(&imgui);

  re_window_destroy(&window);

  re_context_destroy(&g_ctx);

  return 0;
}
