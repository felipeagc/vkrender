#include "light_manager.hpp"
#include <renderer/context.hpp>
#include <renderer/util.hpp>

using namespace engine;

LightManager::LightManager() {
  auto [descriptorPool, descriptorSetLayout] =
      renderer::ctx().m_descriptorManager[renderer::DESC_LIGHTING];

  assert(descriptorPool != nullptr && descriptorSetLayout != nullptr);

  VkDescriptorSetAllocateInfo allocateInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      nullptr,
      *descriptorPool,
      1,
      descriptorSetLayout,
  };

  for (int i = 0; i < renderer::MAX_FRAMES_IN_FLIGHT; i++) {
    vkAllocateDescriptorSets(
        renderer::ctx().m_device, &allocateInfo, &m_descriptorSets[i]);

    m_uniformBuffers[i] = renderer::Buffer{renderer::BufferType::eUniform,
                                           sizeof(LightingUniform)};

    m_uniformBuffers[i].mapMemory(&m_mappings[i]);
    memcpy(m_mappings[i], &m_ubo, sizeof(LightingUniform));

    VkDescriptorBufferInfo bufferInfo = {
        m_uniformBuffers[i].getHandle(), 0, sizeof(LightingUniform)};

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

LightManager::~LightManager() {
  VK_CHECK(vkDeviceWaitIdle(renderer::ctx().m_device));

  for (size_t i = 0; i < ARRAYSIZE(m_uniformBuffers); i++) {
    m_uniformBuffers[i].unmapMemory();
    m_uniformBuffers[i].destroy();
  }

  if (m_descriptorSets[0] != VK_NULL_HANDLE) {
    auto descriptorPool =
        renderer::ctx().m_descriptorManager.getPool(renderer::DESC_LIGHTING);

    assert(descriptorPool != nullptr);

    vkFreeDescriptorSets(
        renderer::ctx().m_device,
        *descriptorPool,
        ARRAYSIZE(m_descriptorSets),
        m_descriptorSets);
  }
}

void LightManager::update(const uint32_t frameIndex) {
  memcpy(m_mappings[frameIndex], &m_ubo, sizeof(LightingUniform));
}

void LightManager::bind(
    renderer::Window &window, renderer::GraphicsPipeline &pipeline) {
  VkCommandBuffer commandBuffer = window.getCurrentCommandBuffer();
  vkCmdBindDescriptorSets(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline.m_pipelineLayout,
      3, // firstSet
      1,
      &m_descriptorSets[window.getCurrentFrameIndex()],
      0,
      nullptr);
}

void LightManager::addLight(const glm::vec3 &pos, const glm::vec3 &color) {
  m_ubo.lights[m_ubo.lightCount] =
      Light{glm::vec4(pos, 1.0), glm::vec4(color, 1.0)};
  m_ubo.lightCount++;
}

void LightManager::resetLights() { m_ubo.lightCount = 0; }
