#include "billboard_component.hpp"
#include <fstl/logging.hpp>
#include <renderer/context.hpp>
#include <renderer/util.hpp>

using namespace engine;

BillboardComponent::BillboardComponent(const renderer::Texture &texture)
    : m_texture(texture) {
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

    VkWriteDescriptorSet descriptorWrites[] = {
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            m_materialDescriptorSets[i],               // dstSet
            0,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &albedoDescriptorInfo,                     // pImageInfo
            nullptr,                                   // pBufferInfo
            nullptr,                                   // pTexelBufferView
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
      *renderer::ctx().m_descriptorManager.getPool(renderer::DESC_MATERIAL),
      ARRAYSIZE(m_materialDescriptorSets),
      m_materialDescriptorSets));

  for (int i = 0; i < renderer::MAX_FRAMES_IN_FLIGHT; i++) {
    m_materialDescriptorSets[i] = VK_NULL_HANDLE;
  }
}

void BillboardComponent::draw(
    renderer::Window &window,
    renderer::GraphicsPipeline &pipeline,
    const glm::mat4 &transform,
    const glm::vec3 &color) {
  m_ubo.model = transform;
  m_ubo.color = glm::vec4(color, 1.0f);

  auto commandBuffer = window.getCurrentCommandBuffer();

  auto i = window.getCurrentFrameIndex();

  vkCmdBindPipeline(
      commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.m_pipeline);

  vkCmdPushConstants(
      commandBuffer,
      pipeline.m_pipelineLayout,
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      0,
      sizeof(BillboardUniform),
      &m_ubo);

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
