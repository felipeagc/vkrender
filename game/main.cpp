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

struct UniformBufferObject {
  glm::vec3 tint;
};

int main() {
  vkr::Window window("Hello");

  vkr::Unique<vkr::StagingBuffer> stagingBuffer{{
      sizeof(uint8_t) * 1000 * 1000,
  }};

  vkr::Unique<vkr::Shader> shader{{
      vkr::Shader::loadCode("../shaders/vert.spv"),
      vkr::Shader::loadCode("../shaders/frag.spv"),
  }};

  std::vector<vkr::DescriptorSetLayoutBinding> descriptorSetLayoutBindings = {{
      0,
      vkr::DescriptorType::eUniformBuffer,
      1,
      vkr::ShaderStageFlagBits::eFragment,
  }};

  vkr::Unique<vkr::DescriptorSetLayout> descriptorSetLayout{
      {descriptorSetLayoutBindings}};

  vkr::VertexFormat vertexFormat =
      vkr::VertexFormatBuilder()
          .addBinding(0, sizeof(Vertex), vkr::VertexInputRate::eVertex)
          .addAttribute(
              0, 0, vkr::Format::eR32G32B32Sfloat, offsetof(Vertex, pos))
          .addAttribute(
              1, 0, vkr::Format::eR32G32B32Sfloat, offsetof(Vertex, color))
          .build();

  vkr::Unique<vkr::GraphicsPipeline> pipeline{{
      window,
      *shader,
      vertexFormat,
      {*descriptorSetLayout},
  }};

  std::array<Vertex, 4> vertices{
      Vertex{{-0.5, -0.5, 0.0}, {1.0, 0.0, 0.0}}, // top left
      Vertex{{0.5, -0.5, 0.0}, {0.0, 1.0, 0.0}},  // top right
      Vertex{{0.5, 0.5, 0.0}, {0.0, 0.0, 1.0}},   // bottom right
      Vertex{{-0.5, 0.5, 0.0}, {0.0, 1.0, 1.0}},  // bottom left
  };

  vkr::Unique<vkr::Buffer> vertexBuffer{{
      sizeof(Vertex) * vertices.size(),
      vkr::BufferUsageFlagBits::eVertexBuffer |
          vkr::BufferUsageFlagBits::eTransferDst,
      vkr::MemoryUsageFlagBits::eGpuOnly,
      vkr::MemoryPropertyFlagBits::eDeviceLocal,
  }};

  stagingBuffer->copyMemory(vertices.data(), sizeof(Vertex) * vertices.size());
  stagingBuffer->transfer(*vertexBuffer, sizeof(Vertex) * vertices.size());

  std::array<uint32_t, 6> indices{0, 1, 2, 2, 3, 0};

  vkr::Unique<vkr::Buffer> indexBuffer{{
      sizeof(uint32_t) * indices.size(),
      vkr::BufferUsageFlagBits::eIndexBuffer |
          vkr::BufferUsageFlagBits::eTransferDst,
      vkr::MemoryUsageFlagBits::eGpuOnly,
      vkr::MemoryPropertyFlagBits::eDeviceLocal,
  }};

  stagingBuffer->copyMemory(indices.data(), sizeof(uint32_t) * indices.size());
  stagingBuffer->transfer(*indexBuffer, sizeof(uint32_t) * indices.size());

  UniformBufferObject ubo{
      {1.0, 0.0, 0.0},
  };

  vkr::Unique<vkr::Buffer> uniformBuffer{{
      sizeof(ubo),
      vkr::BufferUsageFlagBits::eUniformBuffer |
          vkr::BufferUsageFlagBits::eTransferDst,
      vkr::MemoryUsageFlagBits::eGpuOnly,
      vkr::MemoryPropertyFlagBits::eDeviceLocal,
  }};

  stagingBuffer->copyMemory(&ubo, sizeof(ubo));
  stagingBuffer->transfer(*uniformBuffer, sizeof(ubo));

  vkr::Unique<vkr::DescriptorPool> descriptorPool{
      {1, {descriptorSetLayoutBindings}}};

  auto descriptorSets =
      descriptorPool->allocateDescriptorSets(1, *descriptorSetLayout);

  vk::DescriptorBufferInfo bufferInfo = {
      uniformBuffer->getVkBuffer(), 0, sizeof(ubo)};
  vkr::Context::getDevice().updateDescriptorSets(
      vk::WriteDescriptorSet{
          descriptorSets[0],                   // dstSet
          0,                                   // dstBinding
          0,                                   // dstArrayElement
          1,                                   // descriptorCount
          vkr::DescriptorType::eUniformBuffer, // descriptorType
          nullptr,                             // pImageInfo
          &bufferInfo,                         // pBufferInfo
          nullptr,                             // pTexelBufferView
      },
      {});

  while (!window.getShouldClose()) {
    SDL_Event event = window.pollEvent();
    switch (event.type) {
    case SDL_WINDOWEVENT:
      switch (event.window.type) {
      case SDL_WINDOWEVENT_RESIZED:
        window.updateSize(
            static_cast<uint32_t>(event.window.data1),
            static_cast<uint32_t>(event.window.data2));
        break;
      }
      break;
    case SDL_QUIT:
      std::cout << "Goodbye\n";
      window.setShouldClose(true);
      break;
    }

    window.present([&](vkr::CommandBuffer &commandBuffer) {
      commandBuffer.bindGraphicsPipeline(*pipeline);
      commandBuffer.bindIndexBuffer(*indexBuffer, 0, vkr::IndexType::eUint32);
      commandBuffer.bindVertexBuffers(*vertexBuffer);
      commandBuffer.bindDescriptorSets(
          vkr::PipelineBindPoint::eGraphics,
          pipeline->getLayout(),
          0,
          descriptorSets,
          {});
      commandBuffer.drawIndexed(indices.size(), 1, 0, 0, 0);
    });
  }

  return 0;
}
