#include <fstl/fixed_vector.hpp>
#include <fstl/logging.hpp>
#include <glm/glm.hpp>
#include <vkr/buffer.hpp>
#include <vkr/context.hpp>
#include <vkr/graphics_pipeline.hpp>
#include <vkr/shader.hpp>
#include <vkr/vertex_format.hpp>
#include <vkr/window.hpp>

struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;
};

int main() {
  vkr::Window window("Triangle");

  vkr::Shader shader{
      "../shaders/triangle.vert",
      "../shaders/triangle.frag",
  };

  vkr::VertexFormat vertexFormat =
      vkr::VertexFormatBuilder()
          .addBinding(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX)
          .addAttribute(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, pos))
          .addAttribute(
              1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color))
          .build();

  vkr::GraphicsPipeline pipeline{
      window,
      shader,
      vertexFormat,
      fstl::fixed_vector<VkDescriptorSetLayout>{},
  };

  std::array<Vertex, 3> vertices{
      Vertex{{0.5, 0.5}, {0.0, 0.0, 1.0}},
      Vertex{{-0.5, 0.5}, {0.0, 1.0, 0.0}},
      Vertex{{0.0, -0.5}, {1.0, 0.0, 0.0}},
  };

  vkr::Buffer vertexBuffer{
      sizeof(Vertex) * vertices.size(), // size
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
  };

  {
    vkr::StagingBuffer stagingBuffer{sizeof(Vertex) * vertices.size()};

    stagingBuffer.copyMemory(vertices.data(), sizeof(Vertex) * vertices.size());
    stagingBuffer.transfer(vertexBuffer, sizeof(Vertex) * vertices.size());

    stagingBuffer.destroy();
  }

  auto draw = [&]() {
    SDL_Event event = window.pollEvent();

    switch (event.type) {
    case SDL_QUIT:
      window.setShouldClose(true);
      break;
    }

    VkCommandBuffer commandBuffer = window.getCurrentCommandBuffer();

    vkCmdBindPipeline(
        commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getPipeline());

    VkBuffer bufferHandle = vertexBuffer.getHandle();
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &bufferHandle, &offset);

    vkCmdDraw(commandBuffer, vertices.size(), 1, 0, 0);
  };

  while (!window.getShouldClose()) {
    window.present(draw);
  }

  vertexBuffer.destroy();
  pipeline.destroy();
  shader.destroy();
  window.destroy();
  vkr::ctx::destroy();

  return 0;
}
