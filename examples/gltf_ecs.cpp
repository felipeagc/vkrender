#include <ecs/world.hpp>
#include <engine/engine.hpp>
#include <ftl/logging.hpp>
#include <ftl/vector.hpp>
#include <imgui/imgui.h>
#include <renderer/renderer.hpp>
#include <util/file.hpp>

int main() {
  renderer::Context context;
  renderer::Window window("GLTF models", 1600, 900);
  renderer::ImGuiRenderer imgui(window);

  window.clearColor = {0.15, 0.15, 0.15, 1.0};

  engine::Scene scene("../assets/main.sdf");
  auto &world = scene.m_world;
  auto &assetManager = scene.m_assetManager;

  engine::GltfModelSystem gltfModelSystem;
  engine::FPSCameraSystem fpsCameraSystem;
  engine::BillboardSystem billboardSystem;
  engine::SkyboxSystem skyboxSystem;
  engine::LightingSystem lightingSystem;
  engine::EntityInspectorSystem entityInspectorSystem{assetManager};

  re_canvas_t canvas;
  re_canvas_init(&canvas, window.getWidth(), window.getHeight());

  // Create shaders & pipelines
  renderer::GraphicsPipeline model_pipeline;
  eg_init_pipeline(
      &model_pipeline,
      canvas.render_target,
      "../shaders/model_pbr.vert",
      "../shaders/model_pbr.frag",
      engine::standardPipelineParameters());

  renderer::GraphicsPipeline billboard_pipeline;
  eg_init_pipeline(
      &billboard_pipeline,
      canvas.render_target,
      "../shaders/billboard.vert",
      "../shaders/billboard.frag",
      engine::billboardPipelineParameters());

  renderer::GraphicsPipeline wireframe_pipeline;
  eg_init_pipeline(
      &wireframe_pipeline,
      canvas.render_target,
      "../shaders/box.vert",
      "../shaders/box.frag",
      engine::wireframePipelineParameters());

  renderer::GraphicsPipeline skybox_pipeline;
  eg_init_pipeline(
      &skybox_pipeline,
      canvas.render_target,
      "../shaders/skybox.vert",
      "../shaders/skybox.frag",
      engine::skyboxPipelineParameters());

  renderer::GraphicsPipeline fullscreen_pipeline;
  eg_init_pipeline(
      &fullscreen_pipeline,
      window.render_target,
      "../shaders/fullscreen.vert",
      "../shaders/fullscreen.frag",
      engine::fullscreenPipelineParameters());

  float time = 0.0;

  bool drawImgui = false;

  while (!window.getShouldClose()) {
    time += window.getDelta();

    SDL_Event event;
    while (window.pollEvent(&event)) {
      imgui.processEvent(event);
      fpsCameraSystem.processEvent(window, event);
      entityInspectorSystem.processEvent(window, assetManager, world, event);

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
        window.setShouldClose(true);
        break;
      }
    }

    window.beginFrame();

    imgui.begin();

    if (drawImgui) {
      engine::imgui::statsWindow(window);
      engine::imgui::assetsWindow(assetManager);
      entityInspectorSystem.imgui(world);
    }

    lightingSystem.process(window, world);
    fpsCameraSystem.process(window, world);

    // modelShaderWatcher.lockPipeline();

    // Render target pass
    {
      re_canvas_begin(&canvas, window.getCurrentCommandBuffer());

      entityInspectorSystem.drawBox(
          window, assetManager, world, wireframe_pipeline);
      gltfModelSystem.process(window, assetManager, world, model_pipeline);
      skyboxSystem.process(window, world, skybox_pipeline);
      billboardSystem.process(window, world, billboard_pipeline);

      re_canvas_end(&canvas, window.getCurrentCommandBuffer());
    }

    imgui.end();

    // Window pass
    {
      window.beginRenderPass();

      re_canvas_draw(
          &canvas, window.getCurrentCommandBuffer(), &fullscreen_pipeline);

      if (drawImgui) {
        imgui.draw();
      }

      window.endRenderPass();
    }

    window.endFrame();
  }

  re_canvas_destroy(&canvas);

  return 0;
}
