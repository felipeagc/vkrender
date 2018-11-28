#include <engine/engine.hpp>
#include <fstl/fixed_vector.hpp>
#include <fstl/logging.hpp>
#include <renderer/renderer.hpp>

int main() {
  renderer::Window window("GLTF models", 800, 600, VK_SAMPLE_COUNT_1_BIT);

  engine::AssetManager assetManager;

  renderer::Shader modelShader{
      "../shaders/model.vert",
      "../shaders/model.frag",
  };

  renderer::GraphicsPipeline modelPipeline =
      renderer::createStandardPipeline(window, modelShader);

  modelShader.destroy();

  engine::Camera camera{{0.0, 0.0, 0.0}};

  engine::GltfModelInstance helmet{
      assetManager.getAsset<engine::GltfModel>("../assets/helmet_model.json")};
  helmet.m_pos = {0.0, 0.0, 1.0};
  engine::GltfModelInstance duck{
      assetManager.getAsset<engine::GltfModel>("../assets/Duck.glb")};
  duck.m_pos = {0.0, 0.0, -1.0};

  float time = 0.0;

  auto draw = [&]() {
    float radius = 3.0f;
    float camX = sin(time) * radius;
    float camY = cos(time) * radius;
    camera.m_position = {camX, -radius, camY};
    camera.lookAt((helmet.m_pos + duck.m_pos) / 2.0f, {0.0, -1.0, 0.0});

    camera.update(window);

    camera.bind(window, modelPipeline);
    helmet.draw(window, modelPipeline);
    duck.draw(window, modelPipeline);
  };

  while (!window.getShouldClose()) {
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
      case SDL_QUIT:
        fstl::log::info("Goodbye");
        window.setShouldClose(true);
        break;
      }
    }

    window.present(draw);
  }

  return 0;
}
