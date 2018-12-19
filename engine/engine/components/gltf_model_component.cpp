#include "gltf_model_component.hpp"
#include <renderer/context.hpp>
#include <renderer/util.hpp>

using namespace engine;

GltfModelComponent::GltfModelComponent(const GltfModel &model)
    : m_model(model) {
  // Create uniform buffers and descriptors
  {
    auto [descriptorPool, descriptorSetLayout] =
        renderer::ctx().m_descriptorManager[renderer::DESC_MESH];

    assert(descriptorPool != nullptr && descriptorSetLayout != nullptr);

    VkDescriptorSetAllocateInfo allocateInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        nullptr,
        *descriptorPool,
        1,
        descriptorSetLayout,
    };

    for (int i = 0; i < renderer::MAX_FRAMES_IN_FLIGHT; i++) {
      m_uniformBuffers[i] = renderer::Buffer{renderer::BufferType::eUniform,
                                             sizeof(ModelUniform)};

      m_uniformBuffers[i].mapMemory(&m_mappings[i]);

      memcpy(m_mappings[i], &m_ubo, sizeof(ModelUniform));

      VkDescriptorBufferInfo bufferInfo = {
          m_uniformBuffers[i].getHandle(), 0, sizeof(ModelUniform)};

      VK_CHECK(vkAllocateDescriptorSets(
          renderer::ctx().m_device, &allocateInfo, &m_descriptorSets[i]));

      VkWriteDescriptorSet descriptorWrite = {
          VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          nullptr,
          m_descriptorSets[i],               // dstSet
          0,                                 // dstBinding
          0,                                 // dstArrayElement
          1,                                 // descriptorCount
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
          nullptr,                           // pImageInfo
          &bufferInfo,                       // pBufferInfo
          nullptr,                           // pTexelBufferView
      };

      vkUpdateDescriptorSets(
          renderer::ctx().m_device, 1, &descriptorWrite, 0, nullptr);
    }
  }
}

GltfModelComponent::~GltfModelComponent() {
  VK_CHECK(vkDeviceWaitIdle(renderer::ctx().m_device));

  for (size_t i = 0; i < ARRAYSIZE(m_uniformBuffers); i++) {
    m_uniformBuffers[i].unmapMemory();
    m_uniformBuffers[i].destroy();
  }

  auto descriptorPool =
      renderer::ctx().m_descriptorManager.getPool(renderer::DESC_MESH);
  assert(descriptorPool != nullptr);

  vkFreeDescriptorSets(
      renderer::ctx().m_device,
      *descriptorPool,
      ARRAYSIZE(m_descriptorSets),
      m_descriptorSets);
}

void GltfModelComponent::draw(
    renderer::Window &window,
    renderer::GraphicsPipeline &pipeline,
    const glm::mat4 &transform) {
  auto commandBuffer = window.getCurrentCommandBuffer();

  auto i = window.getCurrentFrameIndex();

  // Update model matrix
  m_ubo.model = transform;
  memcpy(m_mappings[i], &m_ubo, sizeof(ModelUniform));

  vkCmdBindPipeline(
      commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.m_pipeline);

  VkDeviceSize offset = 0;
  VkBuffer vertexBuffer = m_model.m_vertexBuffer.getHandle();
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);

  vkCmdBindIndexBuffer(
      commandBuffer,
      m_model.m_indexBuffer.getHandle(),
      0,
      VK_INDEX_TYPE_UINT32);

  vkCmdBindDescriptorSets(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline.m_pipelineLayout,
      3, // firstSet
      1,
      &m_descriptorSets[i],
      0,
      nullptr);

  for (auto &node : m_model.m_nodes) {
    drawNode(node, window, pipeline);
  }
}

void GltfModelComponent::drawNode(
    GltfModel::Node &node,
    renderer::Window &window,
    renderer::GraphicsPipeline &pipeline) {
  VkCommandBuffer commandBuffer = window.getCurrentCommandBuffer();

  auto i = window.getCurrentFrameIndex();

  // TODO: update animations here (when we implement them)

  if (node.meshIndex != -1) {
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline.m_pipelineLayout,
        2, // firstSet
        1,
        &m_model.m_meshes[node.meshIndex].descriptorSets[i],
        0,
        nullptr);

    for (GltfModel::Primitive &primitive :
         m_model.m_meshes[node.meshIndex].primitives) {
      if (primitive.materialIndex != -1 && m_model.m_materials.size() > 0) {
        auto &mat = m_model.m_materials[primitive.materialIndex];
        vkCmdPushConstants(
            commandBuffer,
            pipeline.m_pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(mat.ubo),
            &mat.ubo);

        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline.m_pipelineLayout,
            1, // firstSet
            1,
            &mat.descriptorSets[i],
            0,
            nullptr);
      }

      vkCmdDrawIndexed(
          commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
    }
  }

  for (auto &childIndex : node.childrenIndices) {
    drawNode(m_model.m_nodes[childIndex], window, pipeline);
  }
}