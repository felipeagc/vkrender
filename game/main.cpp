#include <glm/glm.hpp>
#include <iostream>
#include <vkr/buffer.hpp>
#include <vkr/commandbuffer.hpp>
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

  vkr::StagingBuffer stagingBuffer{context, sizeof(uint8_t) * 1000 * 1000};

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
  vkr::GraphicsPipeline pipeline(context, shader, vertexFormat);

  std::array<Vertex, 3> vertices{
      Vertex{{-0.5, 0.5, 0.0}, {1.0, 0.0, 0.0}},
      Vertex{{0.5, 0.5, 0.0}, {0.0, 1.0, 0.0}},
      Vertex{{0.0, -0.5, 0.0}, {0.0, 0.0, 1.0}},
  };

  vkr::Buffer vertexBuffer{context,
                           sizeof(Vertex) * vertices.size(),
                           vkr::BufferUsageFlagBits::eVertexBuffer |
                               vkr::BufferUsageFlagBits::eTransferDst,
                           vkr::MemoryUsageFlagBits::eGpuOnly,
                           vkr::MemoryPropertyFlagBits::eDeviceLocal};

  stagingBuffer.copyMemory(vertices.data(), sizeof(Vertex) * vertices.size());
  stagingBuffer.transfer(vertexBuffer, sizeof(Vertex) * vertices.size());

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

    context.present([&](vkr::CommandBuffer &commandBuffer) {
      commandBuffer.bindGraphicsPipeline(pipeline);
      commandBuffer.bindVertexBuffers(vertexBuffer);
      commandBuffer.draw(vertices.size(), 1, 0, 0);
    });
  }

  vertexBuffer.destroy();
  pipeline.destroy();
  stagingBuffer.destroy();
  shader.destroy();

  return 0;
}
