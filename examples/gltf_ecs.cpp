#include <ecs/world.hpp>
#include <engine/engine.hpp>
#include <fstl/fixed_vector.hpp>
#include <fstl/logging.hpp>
#include <imgui/imgui.h>
#include <renderer/renderer.hpp>

int main() {
  renderer::Context context;
  renderer::Window window("GLTF models", 800, 600, VK_SAMPLE_COUNT_4_BIT);

  window.clearColor = {0.15, 0.15, 0.15, 1.0};

  engine::AssetManager assetManager;

  ecs::World world;
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

  // Create skybox
  ecs::Entity environment = world.createEntity();

  world.assign<engine::EnvironmentComponent>(
      environment,
      assetManager.loadAsset<engine::EnvironmentAsset>(
          static_cast<uint32_t>(1024),
          static_cast<uint32_t>(1024),
          "../assets/ice_lake/irradiance.hdr",
          "../assets/ice_lake/irradiance.hdr",
          std::vector<std::string>{
              "../assets/ice_lake/radiance_0_1600x800.hdr",
              "../assets/ice_lake/radiance_1_800x400.hdr",
              "../assets/ice_lake/radiance_2_400x200.hdr",
              "../assets/ice_lake/radiance_3_200x100.hdr",
              "../assets/ice_lake/radiance_4_100x50.hdr",
              "../assets/ice_lake/radiance_5_50x25.hdr",
              "../assets/ice_lake/radiance_6_25x12.hdr",
              "../assets/ice_lake/radiance_7_12x6.hdr",
              "../assets/ice_lake/radiance_8_6x3.hdr",
          },
          "../assets/brdf_lut.png"));

  // world.assign<engine::EnvironmentComponent>(
  //     skybox,
  //     assetManager.getAsset<engine::CubemapAsset>(
  //         "../assets/park/skybox.hdr",
  //         static_cast<uint32_t>(1024),
  //         static_cast<uint32_t>(1024)),
  //     assetManager.getAsset<engine::CubemapAsset>(
  //         "../assets/park/irradiance.hdr",
  //         static_cast<uint32_t>(1024),
  //         static_cast<uint32_t>(1024)),
  //     assetManager.getAsset<engine::CubemapAsset>(
  //         "mah radiance",
  //         std::vector<std::string>{
  //             "../assets/park/radiance_0_2048x1024.hdr",
  //             "../assets/park/radiance_1_1024x512.hdr",
  //             "../assets/park/radiance_2_512x256.hdr",
  //             "../assets/park/radiance_3_256x128.hdr",
  //             "../assets/park/radiance_4_128x64.hdr",
  //             "../assets/park/radiance_5_64x32.hdr",
  //             "../assets/park/radiance_6_32x16.hdr",
  //             "../assets/park/radiance_7_16x8.hdr",
  //             "../assets/park/radiance_8_8x4.hdr",
  //         },
  //         static_cast<uint32_t>(1024),
  //         static_cast<uint32_t>(1024)),
  //     assetManager.getAsset<engine::TextureAsset>("../assets/brdf_lut.png"));

  // Create lights
  auto &lightAsset =
      assetManager.loadAsset<engine::TextureAsset>("../assets/light.png");
  {
    ecs::Entity light = world.createEntity();
    world.assign<engine::LightComponent>(light, glm::vec3{1.0, 1.0, 0.0});
    world.assign<engine::TransformComponent>(
        light, glm::vec3{3.0, 2.0, 3.0}, glm::vec3{0.5, 0.5, 0.5});
    world.assign<engine::BillboardComponent>(light, lightAsset);
  }
  {
    ecs::Entity light = world.createEntity();
    world.assign<engine::LightComponent>(light, glm::vec3{1.0, 0.0, 0.0});
    world.assign<engine::TransformComponent>(
        light, glm::vec3{-3.0, 2.0, -3.0}, glm::vec3{0.5, 0.5, 0.5});
    world.assign<engine::BillboardComponent>(light, lightAsset);
  }

  // Create models
  ecs::Entity bunny = world.createEntity();
  world.assign<engine::GltfModelComponent>(
      bunny,
      assetManager.loadAsset<engine::GltfModelAsset>("../assets/bunny.glb"));
  world.assign<engine::TransformComponent>(
      bunny,
      glm::vec3{0.0, -1.0, -0.5},
      glm::vec3{1.0},
      glm::angleAxis(glm::radians(-90.0f), glm::vec3{1.0, 0.0, 0.0}));

  ecs::Entity helmet = world.createEntity();
  world.assign<engine::GltfModelComponent>(
      helmet,
      assetManager.loadAsset<engine::GltfModelAsset>(
          "../assets/DamagedHelmet.glb", true));
  world.assign<engine::TransformComponent>(
      helmet,
      glm::vec3{3.0, 0.0, 0.0},
      glm::vec3{1.0},
      glm::angleAxis(glm::radians(-90.0f), glm::vec3{1.0, 0.0, 0.0}));

  ecs::Entity bottle = world.createEntity();
  world.assign<engine::GltfModelComponent>(
      bottle,
      assetManager.loadAsset<engine::GltfModelAsset>(
          "../assets/WaterBottle.glb"));
  world.assign<engine::TransformComponent>(
      bottle, glm::vec3{-3.0, 0.0, 0.0}, glm::vec3{10.0});

  // Create camera
  ecs::Entity camera = world.createEntity();
  world.assign<engine::CameraComponent>(camera);
  world.assign<engine::TransformComponent>(camera, glm::vec3{0.0, 0.0, -5.0});

  float time = 0.0;

  modelShaderWatcher.startWatching();
  billboardShaderWatcher.startWatching();

  while (!window.getShouldClose()) {
    window.beginPresent();

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
    skyboxSystem.process(window, world, skyboxPipeline);
    fpsCameraSystem.process(window, world);
    gltfModelSystem.process(
        window, assetManager, world, modelShaderWatcher.pipeline());
    billboardSystem.process(window, world, billboardShaderWatcher.pipeline());

    window.endPresent();
  }

  return 0;
}
