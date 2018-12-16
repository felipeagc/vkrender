#include "skybox_component.hpp"
#include <fstl/logging.hpp>
#include <renderer/context.hpp>
#include <renderer/util.hpp>

using namespace engine;

SkyboxComponent::SkyboxComponent(
    const renderer::Cubemap &envCubemap,
    const renderer::Cubemap &irradianceCubemap,
    const renderer::Cubemap &radianceCubemap,
    const renderer::Texture &brdfLut)
    : m_envCubemap(envCubemap),
      m_irradianceCubemap(irradianceCubemap),
      m_radianceCubemap(radianceCubemap),
      m_brdfLut(brdfLut) {
  // Allocate material descriptor sets
  {
    auto [descriptorPool, descriptorSetLayout] =
        renderer::ctx().m_descriptorManager[renderer::DESC_ENVIRONMENT];

    assert(descriptorPool != nullptr && descriptorSetLayout != nullptr);

    VkDescriptorSetAllocateInfo allocateInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        nullptr,
        *descriptorPool,
        1,
        descriptorSetLayout,
    };

    for (size_t i = 0; i < ARRAYSIZE(m_descriptorSets); i++) {
      VK_CHECK(vkAllocateDescriptorSets(
          renderer::ctx().m_device, &allocateInfo, &m_descriptorSets[i]));
    }
  }

  // Update descriptor sets
  for (size_t i = 0; i < ARRAYSIZE(m_descriptorSets); i++) {
    m_uniformBuffers[i] = renderer::Buffer{renderer::BufferType::eUniform,
                                           sizeof(EnvironmentUniform)};

    m_uniformBuffers[i].mapMemory(&m_mappings[i]);

    VkDescriptorBufferInfo bufferInfo{
        m_uniformBuffers[i].getHandle(),
        0,
        sizeof(EnvironmentUniform),
    };

    auto envDescriptorInfo = m_envCubemap.getDescriptorInfo();
    auto irradianceDescriptorInfo = m_irradianceCubemap.getDescriptorInfo();
    auto radianceDescriptorInfo = m_radianceCubemap.getDescriptorInfo();
    auto brdfLutDescriptorInfo = m_brdfLut.getDescriptorInfo();

    VkWriteDescriptorSet descriptorWrites[] = {
        VkWriteDescriptorSet{
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
        },
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            m_descriptorSets[i],                       // dstSet
            1,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &envDescriptorInfo,                        // pImageInfo
            nullptr,                                   // pBufferInfo
            nullptr,                                   // pTexelBufferView
        },
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            m_descriptorSets[i],                       // dstSet
            2,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &irradianceDescriptorInfo,                 // pImageInfo
            nullptr,                                   // pBufferInfo
            nullptr,                                   // pTexelBufferView
        },
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            m_descriptorSets[i],                       // dstSet
            3,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &radianceDescriptorInfo,                   // pImageInfo
            nullptr,                                   // pBufferInfo
            nullptr,                                   // pTexelBufferView
        },
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            m_descriptorSets[i],                       // dstSet
            4,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &brdfLutDescriptorInfo,                    // pImageInfo
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

SkyboxComponent::~SkyboxComponent() {
  VK_CHECK(vkDeviceWaitIdle(renderer::ctx().m_device));

  for (size_t i = 0; i < ARRAYSIZE(m_uniformBuffers); i++) {
    m_uniformBuffers[i].unmapMemory();
    m_uniformBuffers[i].destroy();
  }

  VK_CHECK(vkFreeDescriptorSets(
      renderer::ctx().m_device,
      *renderer::ctx().m_descriptorManager.getPool(renderer::DESC_ENVIRONMENT),
      ARRAYSIZE(m_descriptorSets),
      m_descriptorSets));

  for (int i = 0; i < renderer::MAX_FRAMES_IN_FLIGHT; i++) {
    m_descriptorSets[i] = VK_NULL_HANDLE;
  }
}

void SkyboxComponent::bind(
    renderer::Window &window,
    renderer::GraphicsPipeline &pipeline,
    uint32_t setIndex) {
  auto commandBuffer = window.getCurrentCommandBuffer();

  auto i = window.getCurrentFrameIndex();

  vkCmdBindPipeline(
      commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.m_pipeline);

  vkCmdBindDescriptorSets(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline.m_pipelineLayout,
      setIndex, // firstSet
      1,
      &m_descriptorSets[i],
      0,
      nullptr);
}

void SkyboxComponent::draw(
    renderer::Window &window, renderer::GraphicsPipeline &pipeline) {
  auto commandBuffer = window.getCurrentCommandBuffer();

  auto i = window.getCurrentFrameIndex();

  vkCmdBindPipeline(
      commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.m_pipeline);

  vkCmdBindDescriptorSets(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline.m_pipelineLayout,
      1, // firstSet
      1,
      &m_descriptorSets[i],
      0,
      nullptr);

  vkCmdDraw(commandBuffer, 36, 1, 0, 0);
}

void SkyboxComponent::update(renderer::Window &window) {
  auto i = window.getCurrentFrameIndex();
  memcpy(m_mappings[i], &m_environmentUBO, sizeof(EnvironmentUniform));
}
