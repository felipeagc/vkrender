#include <fstl/fixed_vector.hpp>
#include <fstl/logging.hpp>
#include <glm/glm.hpp>
#include <vkr/buffer.hpp>
#include <vkr/camera.hpp>
#include <vkr/commandbuffer.hpp>
#include <vkr/context.hpp>
#include <vkr/gltf_model.hpp>
#include <vkr/graphics_pipeline.hpp>
#include <vkr/shader.hpp>
#include <vkr/texture.hpp>
#include <vkr/window.hpp>

int main() {
  vkr::Window window("GLTF models");

  window.setMSAASamples(vkr::SampleCount::e4);

  vkr::Shader modelShader{
      "../shaders/model.vert",
      "../shaders/model.frag",
  };

  vkr::GraphicsPipeline modelPipeline{
      window,
      modelShader,
      vkr::GltfModel::getVertexFormat(),
      vkr::Context::getDescriptorManager().getDefaultSetLayouts(),
  };

  vkr::Camera camera({3.0, 3.0, 3.0});
  camera.lookAt({0.0, 0.0, 0.0});

  vkr::GltfModel helmet{window, "../assets/DamagedHelmet.glb", true};
  helmet.setPosition({0.0, 0.0, 1.0});
  vkr::GltfModel duck{window, "../assets/Duck.glb"};
  duck.setPosition({0.0, 0.0, -1.0});

  float time = 0.0;

  auto draw = [&]() {
    time += window.getDelta();

    SDL_Event event = window.pollEvent();
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
    window.present(draw);
  }

  helmet.destroy();
  duck.destroy();
  modelPipeline.destroy();
  modelShader.destroy();

  return 0;
}
