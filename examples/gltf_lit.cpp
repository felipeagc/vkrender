#include <fstl/fixed_vector.hpp>
#include <fstl/logging.hpp>
#include <imgui/imgui.h>
#include <renderer/renderer.hpp>
#include <engine/engine.hpp>

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

  // Create light manager
  engine::LightManager lightManager({
      engine::Light{glm::vec4(3.0, 3.0, 3.0, 1.0), glm::vec4(1.0, 0.0, 0.0, 1.0)},
      engine::Light{glm::vec4(-3.0, -3.0, -3.0, 1.0),
                 glm::vec4(0.0, 1.0, 0.0, 1.0)},
  });

  // Create billboards
  fstl::fixed_vector<engine::Billboard> lightBillboards;

  for (uint32_t i = 0; i < lightManager.getLightCount(); i++) {
    lightBillboards.push_back(engine::Billboard{
        assetManager.getAsset<renderer::Texture>("../assets/light.png"), // texture
        {3.0f, 3.0f, 3.0f},                                         // position
        {1.0f, 1.0f, 1.0f},                                         // scale
        {1.0, 0.0, 0.0, 1.0}                                        // color
    });
  }

  // Create models
  engine::GltfModelInstance helmet{
      assetManager.getAsset<engine::GltfModel>("../assets/helmet_model.json")};
  helmet.m_pos = {2.0, 0.0, 0.0};
  engine::GltfModelInstance boombox{
      assetManager.getAsset<engine::GltfModel>("../assets/BoomBox.glb")};
  boombox.m_pos = {-2.0, 0.0, 0.0};
  boombox.m_scale = glm::vec3{1.0, 1.0, 1.0} * 100.0f;

  // Create camera
  engine::Camera camera{{0.0, 0.0, 0.0}};

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
    camera.m_position = {
        camX, cameraRadius * sin(cameraHeightMultiplier), camZ};

    camera.lookAt(
        (helmet.m_pos + boombox.m_pos) / 2.0f,
        glm::normalize(glm::vec3{0.0, -cos(cameraHeightMultiplier), 0.0}));

    // Update camera matrices
    camera.update(window);

    // Show ImGui windows
    engine::imgui::statsWindow(window);
    engine::imgui::assetsWindow(assetManager);
    engine::imgui::lightsWindow(lightManager);
    engine::imgui::cameraWindow(camera);

    // Draw models
    lightManager.bind(window, modelPipeline);
    camera.bind(window, modelPipeline);
    helmet.draw(window, modelPipeline);
    boombox.draw(window, modelPipeline);

    // Draw billboards
    camera.bind(window, billboardPipeline);
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
