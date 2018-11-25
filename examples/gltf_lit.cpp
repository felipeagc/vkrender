#include <fstl/fixed_vector.hpp>
#include <fstl/logging.hpp>
#include <glm/glm.hpp>
#include <imgui/imgui.h>
#include <vkr/asset_manager.hpp>
#include <vkr/billboard.hpp>
#include <vkr/buffer.hpp>
#include <vkr/camera.hpp>
#include <vkr/context.hpp>
#include <vkr/gltf_model_instance.hpp>
#include <vkr/imgui_utils.hpp>
#include <vkr/lighting.hpp>
#include <vkr/pipeline.hpp>
#include <vkr/shader.hpp>
#include <vkr/texture.hpp>
#include <vkr/util.hpp>
#include <vkr/window.hpp>

int main() {
  vkr::Window window("GLTF models", 800, 600, VK_SAMPLE_COUNT_1_BIT);

  window.clearColor = {0.52, 0.80, 0.92, 1.0};

  vkr::AssetManager assetManager;

  // Create shaders & pipelines
  vkr::Shader billboardShader{
      "../shaders/billboard.vert",
      "../shaders/billboard.frag",
  };

  vkr::GraphicsPipeline billboardPipeline =
      vkr::createBillboardPipeline(window, billboardShader);

  billboardShader.destroy();

  vkr::Shader modelShader{
      "../shaders/model_lit.vert",
      "../shaders/model_lit.frag",
  };

  vkr::GraphicsPipeline modelPipeline =
      vkr::createStandardPipeline(window, modelShader);

  modelShader.destroy();

  // Create light manager
  vkr::LightManager lightManager({
      vkr::Light{glm::vec4(3.0, 3.0, 3.0, 1.0), glm::vec4(1.0, 0.0, 0.0, 1.0)},
      vkr::Light{glm::vec4(-3.0, -3.0, -3.0, 1.0),
                 glm::vec4(0.0, 1.0, 0.0, 1.0)},
  });

  // Create billboards
  fstl::fixed_vector<vkr::Billboard> lightBillboards;

  for (uint32_t i = 0; i < lightManager.getLightCount(); i++) {
    lightBillboards.push_back(vkr::Billboard{
        assetManager.getAsset<vkr::Texture>("../assets/light.png"), // texture
        {3.0f, 3.0f, 3.0f},                                         // position
        {1.0f, 1.0f, 1.0f},                                         // scale
        {1.0, 0.0, 0.0, 1.0}                                        // color
    });
  }

  // Create models
  vkr::GltfModelInstance helmet{
      assetManager.getAsset<vkr::GltfModel>("../assets/helmet_model.json")};
  helmet.m_pos = {2.0, 0.0, 0.0};
  vkr::GltfModelInstance boombox{
      assetManager.getAsset<vkr::GltfModel>("../assets/BoomBox.glb")};
  boombox.m_pos = {-2.0, 0.0, 0.0};
  boombox.m_scale = glm::vec3{1.0, 1.0, 1.0} * 100.0f;

  // Create camera
  vkr::Camera camera{{0.0, 0.0, 0.0}};

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
    vkr::imgui::statsWindow(window);
    vkr::imgui::assetsWindow(assetManager);
    vkr::imgui::lightsWindow(lightManager);
    vkr::imgui::cameraWindow(camera);

    // Draw models
    lightManager.bind(window, modelPipeline);
    camera.bind(window, modelPipeline);
    helmet.draw(window, modelPipeline);
    boombox.draw(window, modelPipeline);

    // Draw billboards
    camera.bind(window, billboardPipeline);
    for (uint32_t i = 0; i < lightManager.getLightCount(); i++) {
      vkr::Light light = lightManager.getLights()[i];
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
