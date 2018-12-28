#include <ecs/world.hpp>
#include <engine/engine.hpp>
#include <fstl/fixed_vector.hpp>
#include <fstl/logging.hpp>
#include <imgui/imgui.h>
#include <renderer/renderer.hpp>
#include <scene/driver.hpp>

int main() {
  renderer::Context context;
  renderer::Window window("GLTF models", 800, 600);

  window.clearColor = {0.15, 0.15, 0.15, 1.0};

  engine::Scene scene("../assets/main.scene");
  auto &world = scene.m_world;
  auto &assetManager = scene.m_assetManager;

  engine::GltfModelSystem gltfModelSystem;
  engine::FPSCameraSystem fpsCameraSystem;
  engine::BillboardSystem billboardSystem;
  engine::SkyboxSystem skyboxSystem;
  engine::LightingSystem lightingSystem;
  engine::EntityInspectorSystem entityInspectorSystem;

  // Create shaders & pipelines
  renderer::Shader skyboxShader{
      "../shaders/skybox.vert",
      "../shaders/skybox.frag",
  };

  renderer::GraphicsPipeline skyboxPipeline =
      renderer::SkyboxPipeline(window, skyboxShader);

  skyboxShader.destroy();

  engine::ShaderWatcher<renderer::StandardPipeline> modelShaderWatcher(
      window, "../shaders/model_pbr.vert", "../shaders/model_pbr.frag");

  engine::ShaderWatcher<renderer::StandardPipeline> billboardShaderWatcher(
      window, "../shaders/billboard.vert", "../shaders/billboard.frag");

  float time = 0.0;

  modelShaderWatcher.startWatching();
  billboardShaderWatcher.startWatching();

  while (!window.getShouldClose()) {
    window.beginFrame();

    window.beginRenderPass();

    time += window.getDelta();

    SDL_Event event;
    while (window.pollEvent(&event)) {
      fpsCameraSystem.processEvent(window, event);

      switch (event.type) {
      case SDL_QUIT:
        fstl::log::info("Goodbye");
        window.setShouldClose(true);
        break;
      }
    }

    // Show ImGui windows
    engine::imgui::statsWindow(window);
    engine::imgui::assetsWindow(assetManager);

    modelShaderWatcher.lockPipeline();
    billboardShaderWatcher.lockPipeline();

    entityInspectorSystem.process(world);
    lightingSystem.process(window, world);
    fpsCameraSystem.process(window, world);
    gltfModelSystem.process(
        window, assetManager, world, modelShaderWatcher.pipeline());
    skyboxSystem.process(window, world, skyboxPipeline);
    billboardSystem.process(window, world, billboardShaderWatcher.pipeline());

    window.endRenderPass();

    window.endFrame();
  }

  return 0;
}
