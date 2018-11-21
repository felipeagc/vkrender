#include <fstl/fixed_vector.hpp>
#include <fstl/logging.hpp>
#include <glm/glm.hpp>
#include <vkr/buffer.hpp>
#include <vkr/camera.hpp>
#include <vkr/context.hpp>
#include <vkr/gltf_model.hpp>
#include <vkr/shader.hpp>
#include <vkr/texture.hpp>
#include <vkr/window.hpp>

int main() {
  vkr::Window window("GLTF models", 800, 600, VK_SAMPLE_COUNT_1_BIT);

  vkr::Shader modelShader{
      "../shaders/model.vert",
      "../shaders/model.frag",
  };

  vkr::GraphicsPipeline modelPipeline =
      vkr::createStandardPipeline(window, modelShader);

  modelShader.destroy();

  vkr::Camera camera({3.0, 3.0, 3.0});
  camera.lookAt({0.0, 0.0, 0.0});

  vkr::GltfModel helmet{window, "../assets/DamagedHelmet.glb", true};
  helmet.setPosition({0.0, 0.0, 1.0});
  vkr::GltfModel duck{window, "../assets/Duck.glb"};
  duck.setPosition({0.0, 0.0, -1.0});

  float time = 0.0;

  auto draw = [&]() {
    float radius = 3.0f;
    float camX = sin(time) * radius;
    float camY = cos(time) * radius;
    camera.setPos({camX, radius, camY});
    camera.lookAt((helmet.getPosition() + duck.getPosition()) / 2.0f);

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

  camera.destroy();
  helmet.destroy();
  duck.destroy();
  modelPipeline.destroy();
  window.destroy();
  vkr::ctx::destroy();

  return 0;
}
