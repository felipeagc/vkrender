#include "billboard_component.hpp"
#include <fstl/logging.hpp>
#include <renderer/context.hpp>
#include <renderer/util.hpp>

using namespace engine;

BillboardComponent::BillboardComponent(const renderer::Texture &texture)
    : m_texture(texture) {
  // Create mesh uniform buffers
  for (size_t i = 0; i < ARRAYSIZE(m_meshUniformBuffers); i++) {
    m_meshUniformBuffers[i] =
        renderer::Buffer{renderer::BufferType::eUniform, sizeof(MeshUniform)};
    m_meshUniformBuffers[i].mapMemory(&m_meshMappings[i]);
    memcpy(m_meshMappings[i], &m_meshUbo, sizeof(MeshUniform));
  }

  // Create material uniform buffers
  for (size_t i = 0; i < ARRAYSIZE(m_materialUniformBuffers); i++) {
    m_materialUniformBuffers[i] = renderer::Buffer{
        renderer::BufferType::eUniform, sizeof(MaterialUniform)};

    m_materialUniformBuffers[i].mapMemory(&m_materialMappings[i]);
    memcpy(m_materialMappings[i], &m_materialUbo, sizeof(MaterialUniform));
  }

  // Allocate mesh descriptor sets
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

    for (size_t i = 0; i < ARRAYSIZE(m_meshDescriptorSets); i++) {
      VK_CHECK(vkAllocateDescriptorSets(
          renderer::ctx().m_device, &allocateInfo, &m_meshDescriptorSets[i]));
    }
  }

  // Allocate material descriptor sets
  {
    auto [descriptorPool, descriptorSetLayout] =
        renderer::ctx().m_descriptorManager[renderer::DESC_MATERIAL];

    assert(descriptorPool != nullptr && descriptorSetLayout != nullptr);

    VkDescriptorSetAllocateInfo allocateInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        nullptr,
        *descriptorPool,
        1,
        descriptorSetLayout,
    };

    for (size_t i = 0; i < ARRAYSIZE(m_materialDescriptorSets); i++) {
      VK_CHECK(vkAllocateDescriptorSets(
          renderer::ctx().m_device,
          &allocateInfo,
          &m_materialDescriptorSets[i]));
    }
  }

  // Update descriptor sets
  for (size_t i = 0; i < ARRAYSIZE(m_materialDescriptorSets); i++) {
    auto albedoDescriptorInfo = m_texture.getDescriptorInfo();

    VkDescriptorBufferInfo meshBufferInfo = {
        m_meshUniformBuffers[i].getHandle(), 0, sizeof(MeshUniform)};

    VkDescriptorBufferInfo materialBufferInfo = {
        m_materialUniformBuffers[i].getHandle(), 0, sizeof(MaterialUniform)};

    VkWriteDescriptorSet descriptorWrites[] = {
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            m_materialDescriptorSets[i],       // dstSet
            0,                                 // dstBinding
            0,                                 // dstArrayElement
            1,                                 // descriptorCount
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
            nullptr,                           // pImageInfo
            &materialBufferInfo,               // pBufferInfo
            nullptr,                           // pTexelBufferView
        },
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            m_materialDescriptorSets[i],               // dstSet
            1,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &albedoDescriptorInfo,                     // pImageInfo
            nullptr,                                   // pBufferInfo
            nullptr,                                   // pTexelBufferView
        },
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            m_meshDescriptorSets[i],           // dstSet
            0,                                 // dstBinding
            0,                                 // dstArrayElement
            1,                                 // descriptorCount
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
            nullptr,                           // pImageInfo
            &meshBufferInfo,                   // pBufferInfo
            nullptr,                           // pTexelBufferView
        },
    };

    vkUpdateDescriptorSets(
        renderer::ctx().m_device,
        ARRAYSIZE(descriptorWrites),
        descriptorWrites,
        0,
        nullptr);
  }
}

BillboardComponent::~BillboardComponent() {
  VK_CHECK(vkDeviceWaitIdle(renderer::ctx().m_device));

  VK_CHECK(vkFreeDescriptorSets(
      renderer::ctx().m_device,
      *renderer::ctx().m_descriptorManager.getPool(renderer::DESC_MESH),
      ARRAYSIZE(m_meshDescriptorSets),
      m_meshDescriptorSets));

  for (int i = 0; i < renderer::MAX_FRAMES_IN_FLIGHT; i++) {
    m_meshDescriptorSets[i] = VK_NULL_HANDLE;
  }

  VK_CHECK(vkFreeDescriptorSets(
      renderer::ctx().m_device,
      *renderer::ctx().m_descriptorManager.getPool(renderer::DESC_MATERIAL),
      ARRAYSIZE(m_materialDescriptorSets),
      m_materialDescriptorSets));

  for (int i = 0; i < renderer::MAX_FRAMES_IN_FLIGHT; i++) {
    m_materialDescriptorSets[i] = VK_NULL_HANDLE;
  }

  for (size_t i = 0; i < ARRAYSIZE(m_meshUniformBuffers); i++) {
    m_meshUniformBuffers[i].unmapMemory();
    m_meshUniformBuffers[i].destroy();
  }

  for (size_t i = 0; i < ARRAYSIZE(m_materialUniformBuffers); i++) {
    m_materialUniformBuffers[i].unmapMemory();
    m_materialUniformBuffers[i].destroy();
  }
}

void BillboardComponent::draw(
    renderer::Window &window,
    renderer::GraphicsPipeline &pipeline,
    const glm::mat4 &transform,
    const glm::vec3 &color) {
  m_meshUbo.model = transform;
  m_materialUbo.color = glm::vec4(color, 1.0f);

  memcpy(
      m_meshMappings[window.getCurrentFrameIndex()],
      &m_meshUbo,
      sizeof(MeshUniform));

  memcpy(
      m_materialMappings[window.getCurrentFrameIndex()],
      &m_materialUbo,
      sizeof(MaterialUniform));

  auto commandBuffer = window.getCurrentCommandBuffer();

  auto i = window.getCurrentFrameIndex();

  vkCmdBindPipeline(
      commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.m_pipeline);

  vkCmdBindDescriptorSets(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline.m_pipelineLayout,
      2, // firstSet
      1,
      &m_meshDescriptorSets[i],
      0,
      nullptr);

  vkCmdBindDescriptorSets(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline.m_pipelineLayout,
      1, // firstSet
      1,
      &m_materialDescriptorSets[i],
      0,
      nullptr);

  vkCmdDraw(commandBuffer, 6, 1, 0, 0);
}
