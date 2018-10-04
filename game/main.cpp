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

  vkr::Unique<vkr::StagingBuffer> stagingBuffer{
      {context, sizeof(uint8_t) * 1000 * 1000}};

  vkr::VertexFormat vertexFormat =
      vkr::VertexFormatBuilder()
          .addBinding(0, sizeof(Vertex), vk::VertexInputRate::eVertex)
          .addAttribute(
              0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos))
          .addAttribute(
              1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color))
          .build();

  vkr::Unique<vkr::Shader> shader{
      {context,
       vkr::Shader::loadCode("../shaders/vert.spv"),
       vkr::Shader::loadCode("../shaders/frag.spv")}};
  vkr::Unique<vkr::GraphicsPipeline> pipeline{{context, *shader, vertexFormat}};

  std::array<Vertex, 4> vertices{
    Vertex{{-0.5, -0.5, 0.0}, {1.0, 0.0, 0.0}}, // top left
    Vertex{{0.5, -0.5, 0.0}, {0.0, 1.0, 0.0}}, // top right
    Vertex{{0.5, 0.5, 0.0}, {0.0, 0.0, 1.0}}, // bottom right
    Vertex{{-0.5, 0.5, 0.0}, {0.0, 1.0, 1.0}}, // bottom left
  };

  std::array<uint32_t, 6> indices {
    0, 1, 2, 2, 3, 0
  };

  vkr::Unique<vkr::Buffer> vertexBuffer{
      {context,
       sizeof(Vertex) * vertices.size(),
       vkr::BufferUsageFlagBits::eVertexBuffer |
           vkr::BufferUsageFlagBits::eTransferDst,
       vkr::MemoryUsageFlagBits::eGpuOnly,
       vkr::MemoryPropertyFlagBits::eDeviceLocal}};

  stagingBuffer->copyMemory(vertices.data(), sizeof(Vertex) * vertices.size());
  stagingBuffer->transfer(*vertexBuffer, sizeof(Vertex) * vertices.size());

  vkr::Unique<vkr::Buffer> indexBuffer{
      {context,
       sizeof(uint32_t) * indices.size(),
       vkr::BufferUsageFlagBits::eIndexBuffer |
           vkr::BufferUsageFlagBits::eTransferDst,
       vkr::MemoryUsageFlagBits::eGpuOnly,
       vkr::MemoryPropertyFlagBits::eDeviceLocal}};

  stagingBuffer->copyMemory(indices.data(), sizeof(uint32_t) * indices.size());
  stagingBuffer->transfer(*indexBuffer, sizeof(uint32_t) * indices.size());

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
      commandBuffer.bindGraphicsPipeline(*pipeline);
      commandBuffer.bindIndexBuffer(*indexBuffer, 0, vkr::IndexType::eUint32);
      commandBuffer.bindVertexBuffers(*vertexBuffer);
      commandBuffer.drawIndexed(indices.size(), 1, 0, 0, 0);
    });
  }

  return 0;
}
