#include <ecs/world.hpp>
#include <engine/engine.hpp>
#include <ftl/fixed_vector.hpp>
#include <ftl/logging.hpp>
#include <imgui/imgui.h>
#include <renderer/renderer.hpp>
#include <scene/driver.hpp>

int main() {
  renderer::Context context;
  renderer::Window window("GLTF models", 1600, 900);
  renderer::ImGuiRenderer imgui(window);

  window.clearColor = {0.15, 0.15, 0.15, 1.0};

  engine::Scene scene("../assets/main.scene");
  auto &world = scene.m_world;
  auto &assetManager = scene.m_assetManager;

  engine::GltfModelSystem gltfModelSystem{assetManager};
  engine::FPSCameraSystem fpsCameraSystem;
  engine::BillboardSystem billboardSystem;
  engine::SkyboxSystem skyboxSystem;
  engine::LightingSystem lightingSystem;
  engine::EntityInspectorSystem entityInspectorSystem;

  renderer::RenderTarget renderTarget(window.getWidth(), window.getHeight());

  // Create shaders & pipelines
  renderer::Shader skyboxShader{"../shaders/skybox.vert",
                                "../shaders/skybox.frag"};
  renderer::Shader billboardShader{"../shaders/billboard.vert",
                                   "../shaders/billboard.frag"};
  renderer::Shader boxShader{"../shaders/box.vert", "../shaders/box.frag"};
  renderer::Shader fullscreenShader{"../shaders/fullscreen.vert",
                                    "../shaders/fullscreen.frag"};


  renderer::GraphicsPipeline billboardPipeline =
      renderer::BillboardPipeline(renderTarget, billboardShader);

  renderer::GraphicsPipeline boxPipeline =
      renderer::BoxPipeline(renderTarget, boxShader);

  renderer::GraphicsPipeline skyboxPipeline =
      renderer::SkyboxPipeline(renderTarget, skyboxShader);

  renderer::GraphicsPipeline fullscreenPipeline =
      renderer::FullscreenPipeline(window, fullscreenShader);

  billboardShader.destroy();
  boxShader.destroy();
  skyboxShader.destroy();
  fullscreenShader.destroy();

  engine::ShaderWatcher<renderer::StandardPipeline> modelShaderWatcher(
      renderTarget, "../shaders/model_pbr.vert", "../shaders/model_pbr.frag");

  float time = 0.0;

  bool drawImgui = false;

  modelShaderWatcher.startWatching();

  while (!window.getShouldClose()) {
    time += window.getDelta();

    SDL_Event event;
    while (window.pollEvent(&event)) {
      imgui.processEvent(&event);
      fpsCameraSystem.processEvent(window, event);

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
          renderTarget.resize(
              static_cast<uint32_t>(event.window.data1),
              static_cast<uint32_t>(event.window.data2));
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
      entityInspectorSystem.process(world);
    }

    lightingSystem.process(window, world);
    fpsCameraSystem.process(window, world);

    modelShaderWatcher.lockPipeline();

    // Render target pass
    {
      renderTarget.beginRenderPass(window);

      gltfModelSystem.process(
          window,
          assetManager,
          world,
          modelShaderWatcher.pipeline(),
          boxPipeline);
      skyboxSystem.process(window, world, skyboxPipeline);
      billboardSystem.process(window, world, billboardPipeline);

      renderTarget.endRenderPass(window);
    }

    imgui.end();

    // Window pass
    {
      window.beginRenderPass();

      renderTarget.draw(window, fullscreenPipeline);

      if (drawImgui) {
        imgui.draw();
      }

      window.endRenderPass();
    }

    window.endFrame();
  }

  return 0;
}
