#include <glm/glm.hpp>
#include <iostream>
#include <vkr/context.hpp>
#include <vkr/pipeline.hpp>
#include <vkr/window.hpp>

struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;
};

int main() {
  vkr::Window window("Hello");
  vkr::Context context(window);

  vkr::VertexFormat vertexFormat =
      vkr::VertexFormatBuilder()
          .addBinding(0, sizeof(Vertex), vk::VertexInputRate::eVertex)
          .addAttribute(
              0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos))
          .addAttribute(
              1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color))
          .build();

  vkr::Shader shader(
      context,
      vkr::Shader::loadCode("../shaders/vert.spv"),
      vkr::Shader::loadCode("../shaders/frag.spv"));
  vkr::Pipeline pipeline(context, shader, vertexFormat);

  bool shouldClose = false;

  while (!shouldClose) {
    SDL_Event event = window.pollEvent();
    switch (event.type) {
    case SDL_WINDOWEVENT:
      switch (event.window.type) {
      case SDL_WINDOWEVENT_RESIZED:
        context.updateSize(
            static_cast<uint32_t>(event.window.data1),
            static_cast<uint32_t>(event.window.data2));
        break;
      }
      break;
    case SDL_QUIT:
      std::cout << "Goodbye\n";
      shouldClose = true;
      break;
    }

    context.present([&](vk::CommandBuffer &commandBuffer) {
      commandBuffer.bindPipeline(
          vk::PipelineBindPoint::eGraphics, pipeline.getPipeline());
    });
  }

  pipeline.destroy();
  shader.destroy();

  return 0;
}
