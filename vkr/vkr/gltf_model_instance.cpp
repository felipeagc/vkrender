#include "gltf_model_instance.hpp"
#include "context.hpp"
#include "util.hpp"

using namespace vkr;

GltfModelInstance::GltfModelInstance(const GltfModel &model) : m_model(model) {
  // Create uniform buffers and descriptors
  {
    auto [descriptorPool, descriptorSetLayout] =
        ctx::descriptorManager[DESC_MESH];

    assert(descriptorPool != nullptr && descriptorSetLayout != nullptr);

    VkDescriptorSetAllocateInfo allocateInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        nullptr,
        *descriptorPool,
        1,
        descriptorSetLayout,
    };

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      buffer::createUniformBuffer(
          sizeof(ModelUniform),
          &this->m_uniformBuffers.buffers[i],
          &this->m_uniformBuffers.allocations[i]);

      buffer::mapMemory(
          this->m_uniformBuffers.allocations[i], &this->m_mappings[i]);
      memcpy(this->m_mappings[i], &this->m_ubo, sizeof(ModelUniform));

      VkDescriptorBufferInfo bufferInfo = {
          this->m_uniformBuffers.buffers[i], 0, sizeof(ModelUniform)};

      VK_CHECK(vkAllocateDescriptorSets(
          ctx::device, &allocateInfo, &this->m_descriptorSets[i]));

      VkWriteDescriptorSet descriptorWrite = {
          VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          nullptr,
          this->m_descriptorSets[i],          // dstSet
          0,                                 // dstBinding
          0,                                 // dstArrayElement
          1,                                 // descriptorCount
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
          nullptr,                           // pImageInfo
          &bufferInfo,                       // pBufferInfo
          nullptr,                           // pTexelBufferView
      };

      vkUpdateDescriptorSets(ctx::device, 1, &descriptorWrite, 0, nullptr);
    }
  }
}

void GltfModelInstance::destroy() {
  for (size_t i = 0; i < ARRAYSIZE(m_uniformBuffers.buffers); i++) {
    buffer::unmapMemory(m_uniformBuffers.allocations[i]);
    buffer::destroy(m_uniformBuffers.buffers[i], m_uniformBuffers.allocations[i]);
  }

  auto descriptorPool = ctx::descriptorManager.getPool(DESC_MESH);
  assert(descriptorPool != nullptr);

  vkFreeDescriptorSets(
      ctx::device,
      *descriptorPool,
      m_descriptorSets.size(),
      m_descriptorSets.data());
}

void GltfModelInstance::draw(Window &window, GraphicsPipeline &pipeline) {
  auto commandBuffer = window.getCurrentCommandBuffer();

  auto i = window.getCurrentFrameIndex();

  this->updateUniforms(i);

  vkCmdBindPipeline(
      commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.m_pipeline);

  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(
      commandBuffer, 0, 1, &this->m_model.m_vertexBuffer, &offset);

  vkCmdBindIndexBuffer(
      commandBuffer, this->m_model.m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

  vkCmdBindDescriptorSets(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline.m_pipelineLayout,
      2, // firstSet
      1,
      &this->m_descriptorSets[i],
      0,
      nullptr);

  for (auto &node : this->m_model.m_nodes) {
    drawNode(node, window, pipeline);
  }
}

void GltfModelInstance::updateUniforms(int frameIndex) {
  auto translation = glm::translate(glm::mat4(1.0f), this->m_pos);
  auto rotation = glm::rotate(
      glm::mat4(1.0f), glm::radians(this->m_rotation.x), {1.0, 0.0, 0.0});
  rotation =
      glm::rotate(rotation, glm::radians(this->m_rotation.y), {0.0, 1.0, 0.0});
  rotation =
      glm::rotate(rotation, glm::radians(this->m_rotation.z), {0.0, 0.0, 1.0});
  auto scaling = glm::scale(glm::mat4(1.0f), this->m_scale);

  this->m_ubo.model = translation * rotation * scaling;

  memcpy(this->m_mappings[frameIndex], &this->m_ubo, sizeof(ModelUniform));
}

void GltfModelInstance::drawNode(
    GltfModel::Node &node, Window &window, GraphicsPipeline &pipeline) {
  VkCommandBuffer commandBuffer = window.getCurrentCommandBuffer();

  auto i = window.getCurrentFrameIndex();

  if (node.meshIndex != -1) {
    for (GltfModel::Primitive &primitive :
         this->m_model.m_meshes[node.meshIndex].primitives) {
      if (primitive.materialIndex != -1 && this->m_model.m_materials.size() > 0) {
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline.m_pipelineLayout,
            1, // firstSet
            1,
            &this->m_model.m_materials[primitive.materialIndex].descriptorSets[i],
            0,
            nullptr);
      }

      vkCmdDrawIndexed(
          commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
    }
  }

  for (auto &childIndex : node.childrenIndices) {
    drawNode(this->m_model.m_nodes[childIndex], window, pipeline);
  }
}
