#include <ecs/world.hpp>
#include <engine/engine.hpp>
#include <fstl/fixed_vector.hpp>
#include <fstl/logging.hpp>
#include <imgui/imgui.h>
#include <renderer/renderer.hpp>

inline glm::vec3 lerp(glm::vec3 v1, glm::vec3 v2, float t) {
  return v1 + t * (v2 - v1);
}

int main() {
  renderer::Context context;
  renderer::Window window("GLTF models", 800, 600, VK_SAMPLE_COUNT_1_BIT);

  window.clearColor = {0.15, 0.15, 0.15, 1.0};

  engine::AssetManager assetManager;

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

  ecs::World world;

  // Create skybox
  ecs::Entity skybox = world.createEntity();

  world.assign<engine::SkyboxComponent>(
      skybox,
      assetManager.getAsset<renderer::Cubemap>(
          "../assets/ice_lake/skybox.hdr",
          static_cast<uint32_t>(2048),
          static_cast<uint32_t>(2048)),
      assetManager.getAsset<renderer::Cubemap>(
          "../assets/ice_lake/irradiance.hdr",
          static_cast<uint32_t>(2048),
          static_cast<uint32_t>(2048)),
      assetManager.getAsset<renderer::Cubemap>(
          "mah radiance",
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
          static_cast<uint32_t>(1024),
          static_cast<uint32_t>(1024)),
      assetManager.getAsset<renderer::Texture>("../assets/brdf_lut.png"));

  // world.assign<engine::SkyboxComponent>(
  //     skybox,
  //     assetManager.getAsset<renderer::Cubemap>(
  //         "../assets/park/skybox.hdr",
  //         static_cast<uint32_t>(1024),
  //         static_cast<uint32_t>(1024)),
  //     assetManager.getAsset<renderer::Cubemap>(
  //         "../assets/park/irradiance.hdr",
  //         static_cast<uint32_t>(1024),
  //         static_cast<uint32_t>(1024)),
  //     assetManager.getAsset<renderer::Cubemap>(
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
  //     assetManager.getAsset<renderer::Texture>("../assets/brdf_lut.png"));

  // Create light manager
  engine::LightManager lightManager;

  // Create lights
  {
    ecs::Entity light = world.createEntity();
    world.assign<engine::LightComponent>(light, glm::vec3{1.0, 1.0, 0.0});
    world.assign<engine::TransformComponent>(
        light, glm::vec3{6.0, 2.0, 6.0}, glm::vec3{0.5, 0.5, 0.5});
    world.assign<engine::BillboardComponent>(
        light, assetManager.getAsset<renderer::Texture>("../assets/light.png"));
  }
  {
    ecs::Entity light = world.createEntity();
    world.assign<engine::LightComponent>(light, glm::vec3{1.0, 0.0, 0.0});
    world.assign<engine::TransformComponent>(
        light, glm::vec3{-6.0, 2.0, -6.0}, glm::vec3{0.5, 0.5, 0.5});
    world.assign<engine::BillboardComponent>(
        light, assetManager.getAsset<renderer::Texture>("../assets/light.png"));
  }

  // Create models
  ecs::Entity helmet = world.createEntity();
  world.assign<engine::GltfModelComponent>(
      helmet,
      assetManager.getAsset<engine::GltfModel>(
          "../assets/DamagedHelmet.glb", true));
  world.assign<engine::TransformComponent>(helmet, glm::vec3{5.0, 2.0, 5.0});

  ecs::Entity boombox = world.createEntity();
  world.assign<engine::GltfModelComponent>(
      boombox,
      assetManager.getAsset<engine::GltfModel>(
          "../assets/boombox/BoomBoxWithAxes.gltf"));
  world.assign<engine::TransformComponent>(
      boombox, glm::vec3{-5.0, 2.0, -5.0}, glm::vec3{100.0});

  ecs::Entity bottle = world.createEntity();
  world.assign<engine::GltfModelComponent>(
      bottle,
      assetManager.getAsset<engine::GltfModel>("../assets/WaterBottle.glb"));
  world.assign<engine::TransformComponent>(
      bottle, glm::vec3{0.0, 0.0, 0.0}, glm::vec3{10.0});

  // Create camera
  ecs::Entity camera = world.createEntity();
  world.assign<engine::CameraComponent>(camera);
  world.assign<engine::TransformComponent>(camera, glm::vec3{0.0, 0.0, -5.0});
  glm::vec3 camUp;
  glm::vec3 camFront;
  glm::vec3 camRight;
  float camYaw = glm::radians(90.0f);
  float camPitch = 0.0;

  float time = 0.0;

  int prevMouseX = 0;
  int prevMouseY = 0;

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
      case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_RIGHT &&
            !ImGui::IsAnyItemActive()) {
          window.getMouseState(&prevMouseX, &prevMouseY);
          window.setRelativeMouse(true);
        }
        break;
      case SDL_MOUSEBUTTONUP:
        if (event.button.button == SDL_BUTTON_RIGHT &&
            !ImGui::IsAnyItemActive()) {
          window.setRelativeMouse(false);
          window.warpMouse(prevMouseX, prevMouseY);
        }
        break;
      case SDL_MOUSEMOTION:
        if (event.motion.state & SDL_BUTTON_RMASK) {
          static float sens = 0.07;

          int dx = event.motion.xrel;
          int dy = event.motion.yrel;

          if (window.getRelativeMouse()) {
            camYaw += glm::radians((float)dx) * sens;
            camPitch -= glm::radians((float)dy) * sens;
            camPitch =
                glm::clamp(camPitch, glm::radians(-89.0f), glm::radians(89.0f));
          }
        }
        break;
      case SDL_QUIT:
        fstl::log::info("Goodbye");
        window.setShouldClose(true);
        return;
      }
    }

    // Show ImGui windows
    engine::imgui::statsWindow(window);
    engine::imgui::assetsWindow(assetManager);
    engine::imgui::entitiesWindow(world);

    // Draw skybox
    world.getComponent<engine::CameraComponent>(camera)->bind(
        window, skyboxPipeline);
    world.each<engine::SkyboxComponent>(
        [&](ecs::Entity, engine::SkyboxComponent &skybox) {
          skybox.update(window);
          skybox.draw(window, skyboxPipeline);
        });

    modelShaderWatcher.lockPipeline();
    billboardShaderWatcher.lockPipeline();

    // Camera control
    world.each<engine::CameraComponent, engine::TransformComponent>(
        [&](ecs::Entity,
            engine::CameraComponent &camera,
            engine::TransformComponent &transform) {
          static glm::vec3 cameraTarget;

          camFront.x = cos(camYaw) * cos(camPitch);
          camFront.y = sin(camPitch);
          camFront.z = sin(camYaw) * cos(camPitch);
          camFront = glm::normalize(camFront);

          camRight =
              glm::normalize(glm::cross(camFront, glm::vec3(0.0, 1.0, 0.0)));
          camUp = glm::normalize(glm::cross(camRight, camFront));

          float speed = 10.0f * window.getDelta();
          glm::vec3 movement(0.0);

          if (window.isScancodePressed(renderer::Scancode::eW))
            movement += camFront;
          if (window.isScancodePressed(renderer::Scancode::eS))
            movement -= camFront;
          if (window.isScancodePressed(renderer::Scancode::eA))
            movement -= camRight;
          if (window.isScancodePressed(renderer::Scancode::eD))
            movement += camRight;

          movement *= speed;

          cameraTarget += movement;

          transform.position =
              lerp(transform.position, cameraTarget, window.getDelta() * 10.0f);

          transform.lookAt(camFront, camUp);

          camera.update(window, transform);
        });

    lightManager.resetLights();

    world.each<engine::TransformComponent, engine::LightComponent>(
        [&](ecs::Entity,
            engine::TransformComponent &transform,
            engine::LightComponent &light) {
          lightManager.addLight(transform.position, light.color);
        });

    lightManager.update(window.getCurrentFrameIndex());

    // Draw models
    lightManager.bind(window, modelShaderWatcher.pipeline());

    world.getComponent<engine::CameraComponent>(camera)->bind(
        window, modelShaderWatcher.pipeline());

    world.getComponent<engine::SkyboxComponent>(skybox)->bind(
        window, modelShaderWatcher.pipeline(), 5);

    world.each<engine::GltfModelComponent, engine::TransformComponent>(
        [&](ecs::Entity,
            engine::GltfModelComponent &model,
            engine::TransformComponent &transform) {
          // transform.rotation =
          //     glm::angleAxis(time / 10.0f, glm::vec3(0.0, 1.0, 0.0));
          model.draw(
              window, modelShaderWatcher.pipeline(), transform.getMatrix());
        });

    // Draw billboards
    world.getComponent<engine::CameraComponent>(camera)->bind(
        window, billboardShaderWatcher.pipeline());
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
          window,
          billboardShaderWatcher.pipeline(),
          transform->getMatrix(),
          light->color);
    }
  };

  modelShaderWatcher.startWatching();
  billboardShaderWatcher.startWatching();

  while (!window.getShouldClose()) {
    window.present(draw);
  }

  return 0;
}
