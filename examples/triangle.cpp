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

  VkBuffer vertexBuffer;
  VmaAllocation vertexAllocation;
  vkr::buffer::makeVertexBuffer(
      sizeof(Vertex) * vertices.size(), &vertexBuffer, &vertexAllocation);

  {
    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    vkr::buffer::makeStagingBuffer(
        sizeof(Vertex) * vertices.size(), &stagingBuffer, &stagingAllocation);

    void *stagingMemoryPointer;
    vkr::buffer::mapMemory(stagingAllocation, &stagingMemoryPointer);

    memcpy(
        stagingMemoryPointer,
        vertices.data(),
        sizeof(Vertex) * vertices.size());
    vkr::buffer::bufferTransfer(
        stagingBuffer, vertexBuffer, sizeof(Vertex) * vertices.size());

    vkr::buffer::unmapMemory(stagingAllocation);
    vkr::buffer::destroy(stagingBuffer, stagingAllocation);
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

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);

    vkCmdDraw(commandBuffer, vertices.size(), 1, 0, 0);
  };

  while (!window.getShouldClose()) {
    window.present(draw);
  }

  vkr::buffer::destroy(vertexBuffer, vertexAllocation);
  pipeline.destroy();
  shader.destroy();
  window.destroy();
  vkr::ctx::destroy();

  return 0;
}
