#include <glm/glm.hpp>
#include <iostream>
#include <vkr/buffer.hpp>
#include <vkr/commandbuffer.hpp>
#include <vkr/context.hpp>
#include <vkr/logging.hpp>
#include <vkr/pipeline.hpp>
#include <vkr/texture.hpp>
#include <vkr/window.hpp>

struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec2 texCoords;
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
      "../shaders/shader.vert",
      "../shaders/shader.frag",
  }};

  auto shaderMetadata = shader->getAutoMetadata();

  vkr::Unique<vkr::DescriptorSetLayout> descriptorSetLayout{
      {shaderMetadata.descriptorSetLayoutBindings}};

  vkr::Unique<vkr::GraphicsPipeline> pipeline{{
      window,
      *shader,
      shaderMetadata.vertexFormat,
      {*descriptorSetLayout},
  }};

  std::array<Vertex, 4> vertices{
      Vertex{{-0.5, -0.5, 0.0}, {1.0, 0.0, 0.0}, {0.0, 0.0}}, // top left
      Vertex{{0.5, -0.5, 0.0}, {0.0, 1.0, 0.0}, {1.0, 0.0}},  // top right
      Vertex{{0.5, 0.5, 0.0}, {0.0, 0.0, 1.0}, {1.0, 1.0}},   // bottom right
      Vertex{{-0.5, 0.5, 0.0}, {0.0, 1.0, 1.0}, {0.0, 1.0}},  // bottom left
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

  vkr::Unique<vkr::Texture> texture{{"../assets/texture.png"}};

  vkr::Unique<vkr::DescriptorPool> descriptorPool{
      {1, {shaderMetadata.descriptorSetLayoutBindings}}};

  auto descriptorSets =
      descriptorPool->allocateDescriptorSets(1, *descriptorSetLayout);

  vk::DescriptorBufferInfo bufferInfo{
      uniformBuffer->getVkBuffer(), 0, sizeof(ubo)};
  vk::DescriptorImageInfo imageInfo{
      texture->getSampler(),
      texture->getImageView(),
      vk::ImageLayout::eShaderReadOnlyOptimal,
  };
  vkr::Context::getDevice().updateDescriptorSets(
      {
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
          vk::WriteDescriptorSet{
              descriptorSets[0],                          // dstSet
              1,                                          // dstBinding
              0,                                          // dstArrayElement
              1,                                          // descriptorCount
              vkr::DescriptorType::eCombinedImageSampler, // descriptorType
              &imageInfo,                                 // pImageInfo
              nullptr,                                    // pBufferInfo
              nullptr,                                    // pTexelBufferView
          },
      },
      {});

  while (!window.getShouldClose()) {
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

    window.present([&](vkr::CommandBuffer &commandBuffer) {
      commandBuffer.bindGraphicsPipeline(*pipeline);
      commandBuffer.bindIndexBuffer(*indexBuffer, 0, vkr::IndexType::eUint32);
      commandBuffer.bindVertexBuffer(*vertexBuffer);
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
