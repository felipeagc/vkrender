#include "gltf_model_instance.hpp"
#include "context.hpp"
#include "util.hpp"

using namespace vkr;

GltfModelInstance::GltfModelInstance() {
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    m_uniformBuffers.buffers[i] = VK_NULL_HANDLE;
    m_uniformBuffers.allocations[i] = VK_NULL_HANDLE;
    m_descriptorSets[i] = VK_NULL_HANDLE;
  }
}

GltfModelInstance::GltfModelInstance(const GltfModel &model) : m_model(model) {
  // Create uniform buffers and descriptors
  {
    auto [descriptorPool, descriptorSetLayout] =
        ctx().m_descriptorManager[DESC_MESH];

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
          &m_uniformBuffers.buffers[i],
          &m_uniformBuffers.allocations[i]);

      buffer::mapMemory(m_uniformBuffers.allocations[i], &m_mappings[i]);
      memcpy(m_mappings[i], &m_ubo, sizeof(ModelUniform));

      VkDescriptorBufferInfo bufferInfo = {
          m_uniformBuffers.buffers[i], 0, sizeof(ModelUniform)};

      VK_CHECK(vkAllocateDescriptorSets(
          ctx().m_device, &allocateInfo, &m_descriptorSets[i]));

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

      vkUpdateDescriptorSets(ctx().m_device, 1, &descriptorWrite, 0, nullptr);
    }
  }
}

GltfModelInstance::~GltfModelInstance() {
  VK_CHECK(vkDeviceWaitIdle(ctx().m_device));

  if (m_uniformBuffers.buffers[0] != VK_NULL_HANDLE) {
    for (size_t i = 0; i < ARRAYSIZE(m_uniformBuffers.buffers); i++) {
      buffer::unmapMemory(m_uniformBuffers.allocations[i]);
      buffer::destroy(
          m_uniformBuffers.buffers[i], m_uniformBuffers.allocations[i]);
    }
  }

  auto descriptorPool = ctx().m_descriptorManager.getPool(DESC_MESH);
  assert(descriptorPool != nullptr);

  if (m_descriptorSets[0] != VK_NULL_HANDLE) {
    vkFreeDescriptorSets(
        ctx().m_device,
        *descriptorPool,
        ARRAYSIZE(m_descriptorSets),
        m_descriptorSets);
  }
}

GltfModelInstance::GltfModelInstance(GltfModelInstance &&rhs) {
  m_pos = rhs.m_pos;
  m_scale = rhs.m_scale;
  m_rotation = rhs.m_rotation;

  m_model = rhs.m_model;

  m_ubo = rhs.m_ubo;

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    m_uniformBuffers.buffers[i] = rhs.m_uniformBuffers.buffers[i];
    m_uniformBuffers.allocations[i] = rhs.m_uniformBuffers.allocations[i];
    m_mappings[i] = rhs.m_mappings[i];
    m_descriptorSets[i] = rhs.m_descriptorSets[i];
  }

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    rhs.m_uniformBuffers.buffers[i] = VK_NULL_HANDLE;
    rhs.m_uniformBuffers.allocations[i] = VK_NULL_HANDLE;
    rhs.m_mappings[i] = VK_NULL_HANDLE;
    rhs.m_descriptorSets[i] = VK_NULL_HANDLE;
  }
}

GltfModelInstance &GltfModelInstance::operator=(GltfModelInstance &&rhs) {
  if (this != &rhs) {
    // Free old stuff
    VK_CHECK(vkDeviceWaitIdle(ctx().m_device));

    if (m_uniformBuffers.buffers[0] != VK_NULL_HANDLE) {
      for (size_t i = 0; i < ARRAYSIZE(m_uniformBuffers.buffers); i++) {
        buffer::unmapMemory(m_uniformBuffers.allocations[i]);
        buffer::destroy(
            m_uniformBuffers.buffers[i], m_uniformBuffers.allocations[i]);
      }
    }

    auto descriptorPool = ctx().m_descriptorManager.getPool(DESC_MESH);
    assert(descriptorPool != nullptr);

    if (m_descriptorSets[0] != VK_NULL_HANDLE) {
      vkFreeDescriptorSets(
          ctx().m_device,
          *descriptorPool,
          ARRAYSIZE(m_descriptorSets),
          m_descriptorSets);
    }
  }

  m_pos = rhs.m_pos;
  m_scale = rhs.m_scale;
  m_rotation = rhs.m_rotation;

  m_model = rhs.m_model;

  m_ubo = rhs.m_ubo;

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    m_uniformBuffers.buffers[i] = rhs.m_uniformBuffers.buffers[i];
    m_uniformBuffers.allocations[i] = rhs.m_uniformBuffers.allocations[i];
    m_mappings[i] = rhs.m_mappings[i];
    m_descriptorSets[i] = rhs.m_descriptorSets[i];
  }

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    rhs.m_uniformBuffers.buffers[i] = VK_NULL_HANDLE;
    rhs.m_uniformBuffers.allocations[i] = VK_NULL_HANDLE;
    rhs.m_mappings[i] = VK_NULL_HANDLE;
    rhs.m_descriptorSets[i] = VK_NULL_HANDLE;
  }

  return *this;
}

void GltfModelInstance::draw(Window &window, GraphicsPipeline &pipeline) {
  auto commandBuffer = window.getCurrentCommandBuffer();

  auto i = window.getCurrentFrameIndex();

  this->updateUniforms(i);

  vkCmdBindPipeline(
      commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.m_pipeline);

  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_model.m_vertexBuffer, &offset);

  vkCmdBindIndexBuffer(
      commandBuffer, m_model.m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

  vkCmdBindDescriptorSets(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline.m_pipelineLayout,
      2, // firstSet
      1,
      &m_descriptorSets[i],
      0,
      nullptr);

  for (auto &node : m_model.m_nodes) {
    drawNode(node, window, pipeline);
  }
}

void GltfModelInstance::updateUniforms(int frameIndex) {
  auto translation = glm::translate(glm::mat4(1.0f), m_pos);
  auto rotation =
      glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation.x), {1.0, 0.0, 0.0});
  rotation = glm::rotate(rotation, glm::radians(m_rotation.y), {0.0, 1.0, 0.0});
  rotation = glm::rotate(rotation, glm::radians(m_rotation.z), {0.0, 0.0, 1.0});
  auto scaling = glm::scale(glm::mat4(1.0f), m_scale);

  m_ubo.model = translation * rotation * scaling;

  memcpy(m_mappings[frameIndex], &m_ubo, sizeof(ModelUniform));
}

void GltfModelInstance::drawNode(
    GltfModel::Node &node, Window &window, GraphicsPipeline &pipeline) {
  VkCommandBuffer commandBuffer = window.getCurrentCommandBuffer();

  auto i = window.getCurrentFrameIndex();

  if (node.meshIndex != -1) {
    for (GltfModel::Primitive &primitive :
         m_model.m_meshes[node.meshIndex].primitives) {
      if (primitive.materialIndex != -1 && m_model.m_materials.size() > 0) {
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline.m_pipelineLayout,
            1, // firstSet
            1,
            &m_model.m_materials[primitive.materialIndex].descriptorSets[i],
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
