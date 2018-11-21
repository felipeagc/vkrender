#include "lighting.hpp"
#include "context.hpp"
#include "pipeline.hpp"
#include "util.hpp"

using namespace vkr;

LightManager::LightManager(const fstl::fixed_vector<Light> &lights) {
  this->ubo.lightCount = std::min((uint32_t)lights.size(), MAX_LIGHTS);
  for (uint32_t i = 0; i < this->ubo.lightCount; i++) {
    this->ubo.lights[i].pos = lights[i].pos;
    this->ubo.lights[i].color = lights[i].color;
  }

  for (size_t i = 0; i < ARRAYSIZE(this->uniformBuffers.buffers); i++) {
    vkr::buffer::makeUniformBuffer(
        sizeof(LightingUniform),
        &this->uniformBuffers.buffers[i],
        &this->uniformBuffers.allocations[i]);
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
        ctx::device, &allocateInfo, &this->descriptorSets[i]);

    vkr::buffer::mapMemory(
        this->uniformBuffers.allocations[i], &this->mappings[i]);
    memcpy(this->mappings[i], &this->ubo, sizeof(LightingUniform));

    VkDescriptorBufferInfo bufferInfo = {
        this->uniformBuffers.buffers[i], 0, sizeof(LightingUniform)};

    VkWriteDescriptorSet descriptorWrite = {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        nullptr,
        this->descriptorSets[i],           // dstSet
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
    memcpy(this->mappings[i], &this->ubo, sizeof(LightingUniform));
  }
}

void LightManager::bind(Window &window, GraphicsPipeline &pipeline) {
  VkCommandBuffer commandBuffer = window.getCurrentCommandBuffer();
  vkCmdBindDescriptorSets(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline.pipelineLayout,
      3, // firstSet
      1,
      &this->descriptorSets[window.getCurrentFrameIndex()],
      0,
      nullptr);
}

Light *LightManager::getLights() { return this->ubo.lights; }

uint32_t LightManager::getLightCount() const { return this->ubo.lightCount; }

void LightManager::setLightCount(uint32_t count) {
  this->ubo.lightCount = count;
}

void LightManager::destroy() {
  for (size_t i = 0; i < ARRAYSIZE(this->uniformBuffers.buffers); i++) {
    vkr::buffer::unmapMemory(this->uniformBuffers.allocations[i]);
    vkr::buffer::destroy(
        this->uniformBuffers.buffers[i], this->uniformBuffers.allocations[i]);
  }

  auto descriptorPool = ctx::descriptorManager.getPool(DESC_LIGHTING);

  assert(descriptorPool != nullptr);

  vkFreeDescriptorSets(
      ctx::device,
      *descriptorPool,
      ARRAYSIZE(this->descriptorSets),
      this->descriptorSets);
}
