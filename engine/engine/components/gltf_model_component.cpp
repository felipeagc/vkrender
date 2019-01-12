#include "gltf_model_component.hpp"
#include "../scene.hpp"
#include <renderer/context.hpp>
#include <renderer/util.hpp>
#include <renderer/window.hpp>

using namespace engine;

template <>
void engine::loadComponent<GltfModelComponent>(
    const sdf::Component &comp,
    ecs::World &world,
    AssetManager &assetManager,
    ecs::Entity entity) {
  for (auto &prop : comp.properties) {
    if (strcmp(prop.name, "asset") == 0) {
      uint32_t assetId;
      prop.get_uint32(&assetId);

      world.assign<GltfModelComponent>(
          entity, assetManager.getAsset<GltfModelAsset>(assetId));
    }
  }
}

GltfModelComponent::GltfModelComponent(const GltfModelAsset &modelAsset)
    : m_modelIndex(modelAsset.index) {
  // Create uniform buffers and descriptors
  auto &set_layout = renderer::ctx().resource_manager.set_layouts.mesh;

  for (uint32_t i = 0; i < renderer::MAX_FRAMES_IN_FLIGHT; i++) {
    re_buffer_init_uniform(&m_uniformBuffers[i], sizeof(ModelUniform));

    re_buffer_map_memory(&m_uniformBuffers[i], &m_mappings[i]);

    memcpy(m_mappings[i], &m_ubo, sizeof(ModelUniform));

    VkDescriptorBufferInfo bufferInfo = {
        m_uniformBuffers[i].buffer, 0, sizeof(ModelUniform)};

    this->m_descriptorSets[i] = re_allocate_resource_set(&set_layout);

    VkWriteDescriptorSet descriptorWrite = {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        nullptr,
        m_descriptorSets[i].descriptor_set, // dstSet
        0,                                  // dstBinding
        0,                                  // dstArrayElement
        1,                                  // descriptorCount
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        nullptr,                            // pImageInfo
        &bufferInfo,                        // pBufferInfo
        nullptr,                            // pTexelBufferView
    };

    vkUpdateDescriptorSets(
        renderer::ctx().m_device, 1, &descriptorWrite, 0, nullptr);
  }
}

GltfModelComponent::~GltfModelComponent() {
  VK_CHECK(vkDeviceWaitIdle(renderer::ctx().m_device));

  for (size_t i = 0; i < ARRAYSIZE(m_uniformBuffers); i++) {
    re_buffer_unmap_memory(&m_uniformBuffers[i]);
    re_buffer_destroy(&m_uniformBuffers[i]);
  }

  auto &set_layout = renderer::ctx().resource_manager.set_layouts.mesh;
  for (auto &set : this->m_descriptorSets) {
    re_free_resource_set(&set_layout, &set);
  }
}

void GltfModelComponent::draw(
    const re_window_t *window,
    engine::AssetManager &assetManager,
    re_pipeline_t pipeline,
    const glm::mat4 &transform) {
  auto commandBuffer = re_window_get_current_command_buffer(window);

  auto i = window->current_frame;

  // Update model matrix
  m_ubo.model = transform;
  memcpy(m_mappings[i], &m_ubo, sizeof(ModelUniform));

  vkCmdBindPipeline(
      commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

  auto &gltfModel = assetManager.getAsset<GltfModelAsset>(m_modelIndex);

  VkDeviceSize offset = 0;
  VkBuffer vertexBuffer = gltfModel.m_vertexBuffer.buffer;
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);

  vkCmdBindIndexBuffer(
      commandBuffer, gltfModel.m_indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

  vkCmdBindDescriptorSets(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline.layout,
      3, // firstSet
      1,
      &m_descriptorSets[i].descriptor_set,
      0,
      nullptr);

  for (auto &node : gltfModel.m_nodes) {
    drawNode(gltfModel, node, window, pipeline);
  }
}

void GltfModelComponent::drawNode(
    GltfModelAsset &model,
    GltfModelAsset::Node &node,
    const re_window_t *window,
    re_pipeline_t pipeline) {
  VkCommandBuffer commandBuffer = re_window_get_current_command_buffer(window);

  auto i = window->current_frame;

  // TODO: update animations here (when we implement them)

  if (node.meshIndex != -1) {
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline.layout,
        2, // firstSet
        1,
        &model.m_meshes[node.meshIndex].descriptorSets[i].descriptor_set,
        0,
        nullptr);

    for (GltfModelAsset::Primitive &primitive :
         model.m_meshes[node.meshIndex].primitives) {
      if (primitive.materialIndex != -1 && model.m_materials.size() > 0) {
        auto &mat = model.m_materials[primitive.materialIndex];
        vkCmdPushConstants(
            commandBuffer,
            pipeline.layout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(mat.ubo),
            &mat.ubo);

        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline.layout,
            1, // firstSet
            1,
            &mat.descriptorSets[i].descriptor_set,
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
