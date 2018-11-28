#include <ecs/world.hpp>
#include <engine/engine.hpp>
#include <fstl/fixed_vector.hpp>
#include <fstl/logging.hpp>
#include <imgui/imgui.h>
#include <renderer/renderer.hpp>

int main() {
  renderer::Window window("GLTF models", 800, 600, VK_SAMPLE_COUNT_1_BIT);

  window.clearColor = {0.52, 0.80, 0.92, 1.0};

  engine::AssetManager assetManager;

  // Create shaders & pipelines
  renderer::Shader billboardShader{
      "../shaders/billboard.vert",
      "../shaders/billboard.frag",
  };

  renderer::GraphicsPipeline billboardPipeline =
      renderer::createBillboardPipeline(window, billboardShader);

  billboardShader.destroy();

  renderer::Shader modelShader{
      "../shaders/model_lit.vert",
      "../shaders/model_lit.frag",
  };

  renderer::GraphicsPipeline modelPipeline =
      renderer::createStandardPipeline(window, modelShader);

  modelShader.destroy();

  ecs::World world;

  // Create light manager
  engine::LightManager lightManager({
      engine::Light{glm::vec4(3.0, 3.0, 3.0, 1.0),
                    glm::vec4(1.0, 0.0, 0.0, 1.0)},
      engine::Light{glm::vec4(-3.0, -3.0, -3.0, 1.0),
                    glm::vec4(0.0, 1.0, 0.0, 1.0)},
  });

  // Create billboards
  fstl::fixed_vector<engine::Billboard> lightBillboards;

  for (uint32_t i = 0; i < lightManager.getLightCount(); i++) {
    lightBillboards.push_back(engine::Billboard{
        assetManager.getAsset<renderer::Texture>(
            "../assets/light.png"), // texture
        {3.0f, 3.0f, 3.0f},         // position
        {1.0f, 1.0f, 1.0f},         // scale
        {1.0, 0.0, 0.0, 1.0}        // color
    });
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
        break;
      }
    }

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

    // Show ImGui windows
    engine::imgui::statsWindow(window);
    engine::imgui::assetsWindow(assetManager);
    engine::imgui::lightsWindow(lightManager);
    engine::imgui::cameraWindow(
        world.getComponent<engine::CameraComponent>(camera),
        world.getComponent<engine::TransformComponent>(camera));

    // Draw models
    lightManager.bind(window, modelPipeline);

    world.getComponent<engine::CameraComponent>(camera)->bind(
        window, modelPipeline);

    world.each<engine::GltfModelComponent, engine::TransformComponent>(
        [&](ecs::Entity,
            engine::GltfModelComponent &model,
            engine::TransformComponent &transform) {
          model.draw(window, modelPipeline, transform.getMatrix());
        });

    // Draw billboards
    world.getComponent<engine::CameraComponent>(camera)->bind(
        window, billboardPipeline);
    for (uint32_t i = 0; i < lightManager.getLightCount(); i++) {
      engine::Light light = lightManager.getLights()[i];
      lightBillboards[i].setPos(light.pos);
      lightBillboards[i].setColor(light.color);
      lightBillboards[i].draw(window, billboardPipeline);
    }
  };

  while (!window.getShouldClose()) {
    window.present(draw);
    lightManager.update();
  }

  return 0;
}
