#include <fstl/fixed_vector.hpp>
#include <fstl/unique.hpp>
#include <glm/glm.hpp>
#include <vkr/aliases.hpp>
#include <vkr/buffer.hpp>
#include <vkr/commandbuffer.hpp>
#include <vkr/context.hpp>
#include <vkr/pipeline.hpp>
#include <vkr/window.hpp>

struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;
};

int main() {
  vkr::Window window("Triangle");

  fstl::unique<vkr::Shader> shader{
      "../shaders/triangle.vert",
      "../shaders/triangle.frag",
  };

  vkr::VertexFormat vertexFormat =
      vkr::VertexFormatBuilder()
          .addBinding(0, sizeof(Vertex), vkr::VertexInputRate::eVertex)
          .addAttribute(0, 0, vkr::Format::eR32G32Sfloat, offsetof(Vertex, pos))
          .addAttribute(
              1, 0, vkr::Format::eR32G32B32Sfloat, offsetof(Vertex, color))
          .build();

  fstl::unique<vkr::GraphicsPipeline> pipeline{
      window,
      *shader,
      vertexFormat,
      fstl::fixed_vector<vkr::DescriptorSetLayout>{},
  };

  std::array<Vertex, 3> vertices{
      Vertex{{0.5, 0.5}, {0.0, 0.0, 1.0}},
      Vertex{{-0.5, 0.5}, {0.0, 1.0, 0.0}},
      Vertex{{0.0, -0.5}, {1.0, 0.0, 0.0}},
  };

  fstl::unique<vkr::Buffer> vertexBuffer{
      sizeof(Vertex) * vertices.size(), // size
      vkr::BufferUsageFlagBits::eVertexBuffer |
          vkr::BufferUsageFlagBits::eTransferDst,
      vkr::MemoryUsageFlagBits::eGpuOnly,
      vkr::MemoryPropertyFlagBits::eDeviceLocal,
  };

  {
    vkr::StagingBuffer stagingBuffer{sizeof(Vertex) * vertices.size()};

    stagingBuffer.copyMemory(vertices.data(), sizeof(Vertex) * vertices.size());
    stagingBuffer.transfer(*vertexBuffer, sizeof(Vertex) * vertices.size());

    stagingBuffer.destroy();
  }

  auto draw = [&]() {
    SDL_Event event = window.pollEvent();

    switch (event.type) {
    case SDL_QUIT:
      window.setShouldClose(true);
      break;
    }

    auto commandBuffer = window.getCurrentCommandBuffer();

    commandBuffer.bindGraphicsPipeline(*pipeline);
    commandBuffer.bindVertexBuffers(*vertexBuffer);

    commandBuffer.draw(vertices.size(), 1, 0, 0);
  };

  while (!window.getShouldClose()) {
    window.present(draw);
  }
}
