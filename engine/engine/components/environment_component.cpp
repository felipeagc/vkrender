#include "environment_component.hpp"

#include "../assets/environment_asset.hpp"
#include "../assets/texture_asset.hpp"
#include <fstl/logging.hpp>
#include <renderer/context.hpp>
#include <renderer/util.hpp>

using namespace engine;

EnvironmentComponent::EnvironmentComponent(
    const EnvironmentAsset &environmentAsset)
    : m_environmentAssetIndex(environmentAsset.index()) {
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

    auto skyboxDescriptorInfo =
        environmentAsset.skyboxCubemap().getDescriptorInfo();
    auto irradianceDescriptorInfo =
        environmentAsset.irradianceCubemap().getDescriptorInfo();
    auto radianceDescriptorInfo =
        environmentAsset.radianceCubemap().getDescriptorInfo();
    auto brdfLutDescriptorInfo = environmentAsset.brdfLut().getDescriptorInfo();

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
            &skyboxDescriptorInfo,                     // pImageInfo
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

EnvironmentComponent::~EnvironmentComponent() {
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

void EnvironmentComponent::bind(
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

void EnvironmentComponent::drawSkybox(
    renderer::Window &window, renderer::GraphicsPipeline &pipeline) {
  auto commandBuffer = window.getCurrentCommandBuffer();

  auto i = window.getCurrentFrameIndex();

  this->update(window);

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

void EnvironmentComponent::update(renderer::Window &window) {
  auto i = window.getCurrentFrameIndex();
  memcpy(m_mappings[i], &m_ubo, sizeof(EnvironmentUniform));
}

void EnvironmentComponent::setExposure(float exposure) {
  m_ubo.exposure = exposure;
}

float EnvironmentComponent::getExposure() { return m_ubo.exposure; }

void EnvironmentComponent::addLight(
    const glm::vec3 &pos, const glm::vec3 &color) {
  m_ubo.lights[m_ubo.lightCount] =
      Light{glm::vec4(pos, 1.0), glm::vec4(color, 1.0)};
  m_ubo.lightCount++;
}

void EnvironmentComponent::resetLights() { m_ubo.lightCount = 0; }
