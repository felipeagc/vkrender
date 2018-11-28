#include "light_manager.hpp"
#include <renderer/context.hpp>
#include <renderer/util.hpp>

using namespace engine;

LightManager::LightManager(const fstl::fixed_vector<Light> &lights) {
  m_ubo.lightCount = std::min((uint32_t)lights.size(), MAX_LIGHTS);
  for (uint32_t i = 0; i < m_ubo.lightCount; i++) {
    m_ubo.lights[i].pos = lights[i].pos;
    m_ubo.lights[i].color = lights[i].color;
  }

  for (size_t i = 0; i < ARRAYSIZE(m_uniformBuffers.buffers); i++) {
    renderer::buffer::createUniformBuffer(
        sizeof(LightingUniform),
        &m_uniformBuffers.buffers[i],
        &m_uniformBuffers.allocations[i]);
  }

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

    renderer::buffer::mapMemory(m_uniformBuffers.allocations[i], &m_mappings[i]);
    memcpy(m_mappings[i], &m_ubo, sizeof(LightingUniform));

    VkDescriptorBufferInfo bufferInfo = {
        m_uniformBuffers.buffers[i], 0, sizeof(LightingUniform)};

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

  if (m_uniformBuffers.buffers[0] != VK_NULL_HANDLE) {
    for (size_t i = 0; i < ARRAYSIZE(m_uniformBuffers.buffers); i++) {
      renderer::buffer::unmapMemory(m_uniformBuffers.allocations[i]);
      renderer::buffer::destroy(
          m_uniformBuffers.buffers[i], m_uniformBuffers.allocations[i]);
    }
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

void LightManager::update() {
  for (int i = 0; i < renderer::MAX_FRAMES_IN_FLIGHT; i++) {
    memcpy(m_mappings[i], &m_ubo, sizeof(LightingUniform));
  }
}

void LightManager::bind(renderer::Window &window, renderer::GraphicsPipeline &pipeline) {
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

Light *LightManager::getLights() { return m_ubo.lights; }

uint32_t LightManager::getLightCount() const { return m_ubo.lightCount; }

void LightManager::setLightCount(uint32_t count) { m_ubo.lightCount = count; }
