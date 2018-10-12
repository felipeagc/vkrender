#include <glm/glm.hpp>
#include <iostream>
#include <vkr/buffer.hpp>
#include <vkr/camera.hpp>
#include <vkr/commandbuffer.hpp>
#include <vkr/context.hpp>
#include <vkr/gltf_model.hpp>
#include <vkr/logging.hpp>
#include <vkr/pipeline.hpp>
#include <vkr/texture.hpp>
#include <vkr/window.hpp>

struct UniformBufferObject {
  glm::vec3 tint;
};

int main() {
  vkr::Window window("Hello");

  vkr::Unique<vkr::Shader> modelShader{{
      "../shaders/model.vert",
      "../shaders/model.frag",
  }};

  vkr::Unique<vkr::GraphicsPipeline> modelPipeline{{
      window,
      *modelShader,
      vkr::GltfModel::getVertexFormat(),
      {
          vkr::Context::getDescriptorManager().getCameraSetLayout(),
          vkr::Context::getDescriptorManager().getMaterialSetLayout(),
          vkr::Context::getDescriptorManager().getModelSetLayout(),
      },
  }};

  vkr::Camera camera({3.0, 3.0, 3.0});
  camera.lookAt({0.0, 0.0, 0.0});

  window.setRelativeMouse();

  vkr::Unique<vkr::GltfModel> helmet{{"../assets/DamagedHelmet.glb", true}};
  helmet->setPosition({0.0, 0.0, 1.0});
  vkr::Unique<vkr::GltfModel> duck{{"../assets/Duck.glb"}};
  duck->setPosition({0.0, 0.0, -1.0});

  float time = 0.0;

  while (!window.getShouldClose()) {
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
      vkr::log::info("Goodbye");
      window.setShouldClose(true);
      break;
    }

    float radius = 3.0f;
    float camX = sin(time) * radius;
    float camY = cos(time) * radius;
    camera.setPos({camX, radius, camY});
    camera.lookAt((helmet->getPosition() + duck->getPosition()) / 2.0f);

    camera.update(window.getWidth(), window.getHeight());

    window.present([&](vkr::CommandBuffer &commandBuffer) {
      camera.bind(commandBuffer, *modelPipeline);
      helmet->draw(commandBuffer, *modelPipeline);
      duck->draw(commandBuffer, *modelPipeline);
    });
  }

  return 0;
}
