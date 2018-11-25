#include <fstl/fixed_vector.hpp>
#include <fstl/logging.hpp>
#include <glm/glm.hpp>
#include <vkr/buffer.hpp>
#include <vkr/context.hpp>
#include <vkr/pipeline.hpp>
#include <vkr/shader.hpp>
#include <vkr/util.hpp>
#include <vkr/vertex_format.hpp>
#include <vkr/window.hpp>

struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;
};

vkr::GraphicsPipeline
createTrianglePipeline(vkr::Window &window, vkr::Shader &shader) {
  using namespace vkr;

  auto pipelineLayout = pipeline::createPipelineLayout(0, nullptr);

  auto shaderStageCreateInfos = shader.getPipelineShaderStageCreateInfos();

  vkr::VertexFormat vertexFormat =
      vkr::VertexFormatBuilder()
          .addBinding(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX)
          .addAttribute(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, pos))
          .addAttribute(
              1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color))
          .build();

  auto vertexInputStateCreateInfo =
      vertexFormat.getPipelineVertexInputStateCreateInfo();
  auto inputAssemblyStateCreateInfo = pipeline::defaultInputAssemblyState();
  auto viewportStateCreateInfo = pipeline::defaultViewportState();
  auto rasterizationStateCreateInfo = pipeline::defaultRasterizationState();
  auto multisampleStateCreateInfo =
      pipeline::defaultMultisampleState(window.getMSAASamples());
  auto depthStencilStateCreateInfo = pipeline::defaultDepthStencilState();
  auto colorBlendStateCreateInfo = pipeline::defaultColorBlendState();
  auto dynamicStateCreateInfo = pipeline::defaultDynamicState();

  VkGraphicsPipelineCreateInfo pipelineCreateInfo{
      VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      nullptr,
      0,                                                    // flags
      static_cast<uint32_t>(shaderStageCreateInfos.size()), // stageCount
      shaderStageCreateInfos.data(),                        // pStages
      &vertexInputStateCreateInfo,                          // pVertexInputState
      &inputAssemblyStateCreateInfo, // pInputAssemblyState
      nullptr,                       // pTesselationState
      &viewportStateCreateInfo,      // pViewportState
      &rasterizationStateCreateInfo, // pRasterizationState
      &multisampleStateCreateInfo,   // multisampleState
      &depthStencilStateCreateInfo,  // pDepthStencilState
      &colorBlendStateCreateInfo,    // pColorBlendState
      &dynamicStateCreateInfo,       // pDynamicState
      pipelineLayout,                // pipelineLayout
      window.getRenderPass(),        // renderPass
      0,                             // subpass
      {},                            // basePipelineHandle
      -1                             // basePipelineIndex
  };

  VkPipeline pipeline;

  VK_CHECK(vkCreateGraphicsPipelines(
      ctx().m_device,
      VK_NULL_HANDLE,
      1,
      &pipelineCreateInfo,
      nullptr,
      &pipeline));

  return GraphicsPipeline{pipeline, pipelineLayout};
}

int main() {
  vkr::Window window("Triangle");

  vkr::Shader shader{
      "../shaders/triangle.vert",
      "../shaders/triangle.frag",
  };

  vkr::GraphicsPipeline pipeline = createTrianglePipeline(window, shader);

  shader.destroy();

  std::array<Vertex, 3> vertices{
      Vertex{{0.5, 0.5}, {0.0, 0.0, 1.0}},
      Vertex{{-0.5, 0.5}, {0.0, 1.0, 0.0}},
      Vertex{{0.0, -0.5}, {1.0, 0.0, 0.0}},
  };

  VkBuffer vertexBuffer;
  VmaAllocation vertexAllocation;
  vkr::buffer::createVertexBuffer(
      sizeof(Vertex) * vertices.size(), &vertexBuffer, &vertexAllocation);

  {
    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    vkr::buffer::createStagingBuffer(
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
    VkCommandBuffer commandBuffer = window.getCurrentCommandBuffer();

    vkCmdBindPipeline(
        commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.m_pipeline);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);

    vkCmdDraw(commandBuffer, vertices.size(), 1, 0, 0);
  };

  while (!window.getShouldClose()) {
    SDL_Event event;
    while (window.pollEvent(&event)) {
      switch (event.type) {
      case SDL_QUIT:
        window.setShouldClose(true);
        break;
      }
    }

    window.present(draw);
  }

  vkr::buffer::destroy(vertexBuffer, vertexAllocation);

  return 0;
}
