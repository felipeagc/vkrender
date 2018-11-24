#include "lighting.hpp"
#include "context.hpp"
#include "pipeline.hpp"
#include "util.hpp"

using namespace vkr;

LightManager::LightManager(const fstl::fixed_vector<Light> &lights) {
  this->m_ubo.lightCount = std::min((uint32_t)lights.size(), MAX_LIGHTS);
  for (uint32_t i = 0; i < this->m_ubo.lightCount; i++) {
    this->m_ubo.lights[i].pos = lights[i].pos;
    this->m_ubo.lights[i].color = lights[i].color;
  }

  for (size_t i = 0; i < ARRAYSIZE(this->m_uniformBuffers.buffers); i++) {
    vkr::buffer::createUniformBuffer(
        sizeof(LightingUniform),
        &this->m_uniformBuffers.buffers[i],
        &this->m_uniformBuffers.allocations[i]);
  }

  auto [descriptorPool, descriptorSetLayout] =
      ctx::descriptorManager[DESC_LIGHTING];

  assert(descriptorPool != nullptr && descriptorSetLayout != nullptr);

  VkDescriptorSetAllocateInfo allocateInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      nullptr,
      *descriptorPool,
      1,
      descriptorSetLayout,
  };

  for (int i = 0; i < vkr::MAX_FRAMES_IN_FLIGHT; i++) {
    vkAllocateDescriptorSets(
        ctx::device, &allocateInfo, &this->m_descriptorSets[i]);

    vkr::buffer::mapMemory(
        this->m_uniformBuffers.allocations[i], &this->m_mappings[i]);
    memcpy(this->m_mappings[i], &this->m_ubo, sizeof(LightingUniform));

    VkDescriptorBufferInfo bufferInfo = {
        this->m_uniformBuffers.buffers[i], 0, sizeof(LightingUniform)};

    VkWriteDescriptorSet descriptorWrite = {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        nullptr,
        this->m_descriptorSets[i],           // dstSet
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

void LightManager::update() {
  for (int i = 0; i < vkr::MAX_FRAMES_IN_FLIGHT; i++) {
    memcpy(this->m_mappings[i], &this->m_ubo, sizeof(LightingUniform));
  }
}

void LightManager::bind(Window &window, GraphicsPipeline &pipeline) {
  VkCommandBuffer commandBuffer = window.getCurrentCommandBuffer();
  vkCmdBindDescriptorSets(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline.m_pipelineLayout,
      3, // firstSet
      1,
      &this->m_descriptorSets[window.getCurrentFrameIndex()],
      0,
      nullptr);
}

Light *LightManager::getLights() { return this->m_ubo.lights; }

uint32_t LightManager::getLightCount() const { return this->m_ubo.lightCount; }

void LightManager::setLightCount(uint32_t count) {
  this->m_ubo.lightCount = count;
}

void LightManager::destroy() {
  for (size_t i = 0; i < ARRAYSIZE(this->m_uniformBuffers.buffers); i++) {
    vkr::buffer::unmapMemory(this->m_uniformBuffers.allocations[i]);
    vkr::buffer::destroy(
        this->m_uniformBuffers.buffers[i], this->m_uniformBuffers.allocations[i]);
  }

  auto descriptorPool = ctx::descriptorManager.getPool(DESC_LIGHTING);

  assert(descriptorPool != nullptr);

  vkFreeDescriptorSets(
      ctx::device,
      *descriptorPool,
      ARRAYSIZE(this->m_descriptorSets),
      this->m_descriptorSets);
}
