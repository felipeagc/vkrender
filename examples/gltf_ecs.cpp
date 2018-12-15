#include <ecs/world.hpp>
#include <engine/engine.hpp>
#include <fstl/fixed_vector.hpp>
#include <fstl/logging.hpp>
#include <imgui/imgui.h>
#include <mutex>
#include <renderer/renderer.hpp>
#include <thread>

int main() {
  renderer::Context context;
  renderer::Window window("GLTF models", 800, 600, VK_SAMPLE_COUNT_1_BIT);

  window.clearColor = {0.15, 0.15, 0.15, 1.0};

  engine::AssetManager assetManager;

  // Create shaders & pipelines
  renderer::Shader billboardShader{
      "../shaders/billboard.vert",
      "../shaders/billboard.frag",
  };

  renderer::GraphicsPipeline billboardPipeline =
      renderer::BillboardPipeline(window, billboardShader);

  billboardShader.destroy();

  renderer::Shader skyboxShader{
      "../shaders/skybox.vert",
      "../shaders/skybox.frag",
  };

  renderer::GraphicsPipeline skyboxPipeline =
      renderer::SkyboxPipeline(window, skyboxShader);

  skyboxShader.destroy();

  engine::ShaderWatcher<renderer::StandardPipeline> shaderWatcher(
      window, "../shaders/model_pbr.vert", "../shaders/model_pbr.frag");

  ecs::World world;

  // Create skybox
  {
    ecs::Entity skybox = world.createEntity();
    world.assign<engine::SkyboxComponent>(
        skybox,
        assetManager.getAsset<renderer::Cubemap>(
            "../assets/ice_lake_ref.hdr",
            static_cast<uint32_t>(1024),
            static_cast<uint32_t>(1024)));
  }

  // Create light manager
  engine::LightManager lightManager;

  // Create lights
  {
    ecs::Entity light = world.createEntity();
    world.assign<engine::LightComponent>(light, glm::vec3{1.0, 1.0, 0.0});
    world.assign<engine::TransformComponent>(
        light, glm::vec3{3.0, 3.0, 3.0}, glm::vec3{0.5, 0.5, 0.5});
    world.assign<engine::BillboardComponent>(
        light, assetManager.getAsset<renderer::Texture>("../assets/light.png"));
  }
  {
    ecs::Entity light = world.createEntity();
    world.assign<engine::LightComponent>(light, glm::vec3{0.0, 0.0, 1.0});
    world.assign<engine::TransformComponent>(
        light, glm::vec3{-3.0, -3.0, -3.0}, glm::vec3{0.5, 0.5, 0.5});
    world.assign<engine::BillboardComponent>(
        light, assetManager.getAsset<renderer::Texture>("../assets/light.png"));
  }

  // Create models
  ecs::Entity helmet = world.createEntity();
  world.assign<engine::GltfModelComponent>(
      helmet,
      assetManager.getAsset<engine::GltfModel>(
          "../assets/DamagedHelmet.glb", true));
  world.assign<engine::TransformComponent>(helmet, glm::vec3{2.0, 0.0, 0.0});

  ecs::Entity boombox = world.createEntity();
  world.assign<engine::GltfModelComponent>(
      boombox,
      assetManager.getAsset<engine::GltfModel>("../assets/BoomBox.glb"));
  world.assign<engine::TransformComponent>(
      boombox, glm::vec3{-2.0, 0.0, 0.0}, glm::vec3{1.0, 1.0, 1.0} * 100.0f);

  ecs::Entity bunny = world.createEntity();
  world.assign<engine::GltfModelComponent>(
      bunny,
      assetManager.getAsset<engine::GltfModel>(
          "../assets/DamagedHelmet.glb", true));
  world.assign<engine::TransformComponent>(bunny, glm::vec3{0.0, 1.0, 0.0});

  // Create camera
  ecs::Entity camera = world.createEntity();
  world.assign<engine::CameraComponent>(camera);
  world.assign<engine::TransformComponent>(camera);

  float time = 0.0;
  float cameraAngle = 0;
  float cameraHeightMultiplier = 0;
  float cameraRadius = 6.0f;

  auto draw = [&]() {
    time += window.getDelta();

    SDL_Event event;
    while (window.pollEvent(&event)) {
      switch (event.type) {
      case SDL_WINDOWEVENT:
        switch (event.window.type) {
        case SDL_WINDOWEVENT_RESIZED:
          window.updateSize();
          break;
        }
        break;
      case SDL_MOUSEMOTION:
        if (event.motion.state & SDL_BUTTON_LMASK &&
            !ImGui::IsAnyItemActive()) {
          cameraAngle -= (float)event.motion.xrel / 100.0f;
          cameraHeightMultiplier -= (float)event.motion.yrel / 100.0f;
        }
        break;
      case SDL_MOUSEWHEEL:
        cameraRadius -= event.wheel.y;
        if (cameraRadius < 0.0f) {
          cameraRadius = 0.0f;
        }
        break;
      case SDL_QUIT:
        fstl::log::info("Goodbye");
        window.setShouldClose(true);
        return;
      }
    }

    // Draw skybox
    world.getComponent<engine::CameraComponent>(camera)->bind(
        window, skyboxPipeline);
    world.each<engine::SkyboxComponent>(
        [&](ecs::Entity, engine::SkyboxComponent &skybox) {
          skybox.draw(window, skyboxPipeline);
        });

    shaderWatcher.lockPipeline();

    // Change camera position and rotation
    float camX = sin(cameraAngle) * cameraRadius * cos(cameraHeightMultiplier);
    float camZ = -cos(cameraAngle) * cameraRadius * cos(cameraHeightMultiplier);
    world.each<engine::CameraComponent, engine::TransformComponent>(
        [&](ecs::Entity,
            engine::CameraComponent &camera,
            engine::TransformComponent &transform) {
          transform.position = {
              camX, cameraRadius * sin(cameraHeightMultiplier), camZ};

          transform.lookAt(
              {0.0, 0.0, 0.0},
              glm::normalize(
                  glm::vec3{0.0, -cos(cameraHeightMultiplier), 0.0}));

          camera.update(window, transform.getMatrix());
        });

    lightManager.resetLights();

    ImGui::Begin("Lights");

    static bool drawBillboards = true;
    ImGui::Checkbox("Draw billboards", &drawBillboards);

    ImGui::Separator();

    world.each<engine::TransformComponent, engine::LightComponent>(
        [&](ecs::Entity entity,
            engine::TransformComponent &transform,
            engine::LightComponent &light) {
          lightManager.addLight(transform.position, light.color);
          engine::imgui::lightSection(entity, transform, light);
        });

    ImGui::End();

    lightManager.update(window.getCurrentFrameIndex());

    // Show ImGui windows
    engine::imgui::statsWindow(window);
    engine::imgui::assetsWindow(assetManager);
    engine::imgui::cameraWindow(
        world.getComponent<engine::CameraComponent>(camera),
        world.getComponent<engine::TransformComponent>(camera));

    // Draw models
    lightManager.bind(window, shaderWatcher.pipeline());

    world.getComponent<engine::CameraComponent>(camera)->bind(
        window, shaderWatcher.pipeline());

    world.each<engine::GltfModelComponent, engine::TransformComponent>(
        [&](ecs::Entity,
            engine::GltfModelComponent &model,
            engine::TransformComponent &transform) {
          model.draw(window, shaderWatcher.pipeline(), transform.getMatrix());
        });

    // Draw billboards
    if (drawBillboards) {
      world.getComponent<engine::CameraComponent>(camera)->bind(
          window, billboardPipeline);
      glm::vec3 cameraPos =
          world.getComponent<engine::TransformComponent>(camera)->position;
      fstl::fixed_vector<std::pair<float, ecs::Entity>> billboards;
      world.each<
          engine::TransformComponent,
          engine::LightComponent,
          engine::BillboardComponent>([&](ecs::Entity entity,
                                          engine::TransformComponent &transform,
                                          engine::LightComponent &,
                                          engine::BillboardComponent &) {
        billboards.push_back(
            {glm::distance(cameraPos, transform.position), entity});
      });

      // Sort draw calls
      std::sort(billboards.begin(), billboards.end());

      for (auto &[dist, entity] : billboards) {
        auto transform = world.getComponent<engine::TransformComponent>(entity);
        auto light = world.getComponent<engine::LightComponent>(entity);
        auto billboard = world.getComponent<engine::BillboardComponent>(entity);
        billboard->draw(
            window, billboardPipeline, transform->getMatrix(), light->color);
      }
    }
  };

  shaderWatcher.startWatching();

  while (!window.getShouldClose()) {
    window.present(draw);
  }

  return 0;
}
