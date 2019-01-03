#include <ftl/fixed_vector.hpp>
#include <ftl/logging.hpp>
#include <renderer/renderer.hpp>

struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;
};

renderer::GraphicsPipeline
createTrianglePipeline(renderer::Window &window, renderer::Shader &shader) {
  using namespace renderer;

  auto pipelineLayout = pipeline::createPipelineLayout(0, nullptr);

  auto shaderStageCreateInfos = shader.getPipelineShaderStageCreateInfos();

  renderer::VertexFormat vertexFormat =
      renderer::VertexFormatBuilder()
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
      pipeline::defaultMultisampleState(window.getSampleCount());
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
  renderer::Context context;
  renderer::Window window("Triangle");

  renderer::Shader shader{
      "../shaders/triangle.vert",
      "../shaders/triangle.frag",
  };

  renderer::GraphicsPipeline pipeline = createTrianglePipeline(window, shader);

  shader.destroy();

  std::array<Vertex, 3> vertices{
      Vertex{{0.5, 0.5}, {0.0, 0.0, 1.0}},
      Vertex{{-0.5, 0.5}, {0.0, 1.0, 0.0}},
      Vertex{{0.0, -0.5}, {1.0, 0.0, 0.0}},
  };

  renderer::Buffer vertexBuffer{renderer::BufferType::eVertex,
                                sizeof(Vertex) * vertices.size()};

  {
    renderer::Buffer stagingBuffer{renderer::BufferType::eStaging,
                                   sizeof(Vertex) * vertices.size()};

    void *stagingMemoryPointer;
    stagingBuffer.mapMemory(&stagingMemoryPointer);

    memcpy(
        stagingMemoryPointer,
        vertices.data(),
        sizeof(Vertex) * vertices.size());
    stagingBuffer.bufferTransfer(
        vertexBuffer, sizeof(Vertex) * vertices.size());

    stagingBuffer.unmapMemory();
    stagingBuffer.destroy();
  }

  while (!window.getShouldClose()) {
    window.beginFrame();
    window.beginRenderPass();

    SDL_Event event;
    while (window.pollEvent(&event)) {
      switch (event.type) {
      case SDL_QUIT:
        window.setShouldClose(true);
        break;
      }
    }

    VkCommandBuffer commandBuffer = window.getCurrentCommandBuffer();

    vkCmdBindPipeline(
        commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.m_pipeline);

    VkDeviceSize offset = 0;
    VkBuffer vbuffer = vertexBuffer.getHandle();
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbuffer, &offset);

    vkCmdDraw(commandBuffer, vertices.size(), 1, 0, 0);

    window.endRenderPass();
    window.endFrame();
  }

  vertexBuffer.destroy();

  return 0;
}
