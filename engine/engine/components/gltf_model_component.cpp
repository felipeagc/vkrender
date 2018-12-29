#include "gltf_model_component.hpp"
#include "../scene.hpp"
#include <renderer/context.hpp>
#include <renderer/util.hpp>
#include <renderer/window.hpp>

using namespace engine;

template <>
void engine::loadComponent<GltfModelComponent>(
    const scene::Component &comp,
    ecs::World &world,
    AssetManager &assetManager,
    ecs::Entity entity) {
  world.assign<GltfModelComponent>(
      entity,
      assetManager.getAsset<engine::GltfModelAsset>(
          comp.properties.at("asset").getUint32()));
}

GltfModelComponent::GltfModelComponent(const GltfModelAsset &modelAsset)
    : m_modelIndex(modelAsset.index()) {
  // Create uniform buffers and descriptors
  auto &setLayout = renderer::ctx().m_resourceManager.m_setLayouts.mesh;

  for (uint32_t i = 0; i < renderer::MAX_FRAMES_IN_FLIGHT; i++) {
    m_uniformBuffers[i] =
        renderer::Buffer{renderer::BufferType::eUniform, sizeof(ModelUniform)};

    m_uniformBuffers[i].mapMemory(&m_mappings[i]);

    memcpy(m_mappings[i], &m_ubo, sizeof(ModelUniform));

    VkDescriptorBufferInfo bufferInfo = {
        m_uniformBuffers[i].getHandle(), 0, sizeof(ModelUniform)};

    this->m_descriptorSets[i] = setLayout.allocate();

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

GltfModelComponent::~GltfModelComponent() {
  VK_CHECK(vkDeviceWaitIdle(renderer::ctx().m_device));

  for (size_t i = 0; i < ARRAYSIZE(m_uniformBuffers); i++) {
    m_uniformBuffers[i].unmapMemory();
    m_uniformBuffers[i].destroy();
  }

  auto &setLayout = renderer::ctx().m_resourceManager.m_setLayouts.mesh;
  for (auto &set : this->m_descriptorSets) {
    setLayout.free(set);
  }
}

void GltfModelComponent::draw(
    renderer::Window &window,
    engine::AssetManager &assetManager,
    renderer::GraphicsPipeline &pipeline,
    const glm::mat4 &transform) {
  auto commandBuffer = window.getCurrentCommandBuffer();

  auto i = window.getCurrentFrameIndex();

  // Update model matrix
  m_ubo.model = transform;
  memcpy(m_mappings[i], &m_ubo, sizeof(ModelUniform));

  vkCmdBindPipeline(
      commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.m_pipeline);

  auto &gltfModel = assetManager.getAsset<GltfModelAsset>(m_modelIndex);

  VkDeviceSize offset = 0;
  VkBuffer vertexBuffer = gltfModel.m_vertexBuffer.getHandle();
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);

  vkCmdBindIndexBuffer(
      commandBuffer,
      gltfModel.m_indexBuffer.getHandle(),
      0,
      VK_INDEX_TYPE_UINT32);

  vkCmdBindDescriptorSets(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline.m_pipelineLayout,
      3, // firstSet
      1,
      m_descriptorSets[i],
      0,
      nullptr);

  for (auto &node : gltfModel.m_nodes) {
    drawNode(gltfModel, node, window, pipeline);
  }
}

void GltfModelComponent::drawNode(
    GltfModelAsset &model,
    GltfModelAsset::Node &node,
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
        model.m_meshes[node.meshIndex].descriptorSets[i],
        0,
        nullptr);

    for (GltfModelAsset::Primitive &primitive :
         model.m_meshes[node.meshIndex].primitives) {
      if (primitive.materialIndex != -1 && model.m_materials.size() > 0) {
        auto &mat = model.m_materials[primitive.materialIndex];
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
            mat.descriptorSets[i],
            0,
            nullptr);
      }

      vkCmdDrawIndexed(
          commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
    }
  }

  for (auto &childIndex : node.childrenIndices) {
    drawNode(model, model.m_nodes[childIndex], window, pipeline);
  }
}
