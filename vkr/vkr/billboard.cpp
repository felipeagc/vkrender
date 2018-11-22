#include "billboard.hpp"
#include "context.hpp"
#include "pipeline.hpp"
#include "util.hpp"

using namespace vkr;

Billboard::Billboard(
    const Texture &texture, glm::vec3 pos, glm::vec3 scale, glm::vec4 color)
    : texture_(texture) {

  // Initialize mesh UBO struct
  glm::mat4 model(1.0f);
  model = glm::translate(model, pos);
  model = glm::scale(model, scale);
  this->meshUbo_.model = model;

  // Initialize material UBO struct
  this->materialUbo_.color = color;

  // Create mesh uniform buffers
  for (size_t i = 0; i < ARRAYSIZE(this->meshUniformBuffers_.buffers); i++) {
    buffer::makeUniformBuffer(
        sizeof(MeshUniform),
        &this->meshUniformBuffers_.buffers[i],
        &this->meshUniformBuffers_.allocations[i]);

    buffer::mapMemory(
        this->meshUniformBuffers_.allocations[i], &this->meshMappings_[i]);
    memcpy(this->meshMappings_[i], &this->meshUbo_, sizeof(MeshUniform));
  }

  // Create material uniform buffers
  for (size_t i = 0; i < ARRAYSIZE(this->materialUniformBuffers_.buffers);
       i++) {
    buffer::makeUniformBuffer(
        sizeof(MaterialUniform),
        &this->materialUniformBuffers_.buffers[i],
        &this->materialUniformBuffers_.allocations[i]);

    buffer::mapMemory(
        this->materialUniformBuffers_.allocations[i],
        &this->materialMappings_[i]);
    memcpy(
        this->materialMappings_[i],
        &this->materialUbo_,
        sizeof(MaterialUniform));
  }

  // Allocate mesh descriptor sets
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

    for (size_t i = 0; i < ARRAYSIZE(this->meshDescriptorSets_); i++) {
      VK_CHECK(vkAllocateDescriptorSets(
          ctx::device, &allocateInfo, &this->meshDescriptorSets_[i]));
    }
  }

  // Allocate material descriptor sets
  {
    auto [descriptorPool, descriptorSetLayout] =
        ctx::descriptorManager[DESC_MATERIAL];

    assert(descriptorPool != nullptr && descriptorSetLayout != nullptr);

    VkDescriptorSetAllocateInfo allocateInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        nullptr,
        *descriptorPool,
        1,
        descriptorSetLayout,
    };

    for (size_t i = 0; i < ARRAYSIZE(this->materialDescriptorSets_); i++) {
      VK_CHECK(vkAllocateDescriptorSets(
          ctx::device, &allocateInfo, &this->materialDescriptorSets_[i]));
    }
  }

  // Update descriptor sets
  for (size_t i = 0; i < ARRAYSIZE(this->materialDescriptorSets_); i++) {
    auto albedoDescriptorInfo = this->texture_.getDescriptorInfo();

    VkDescriptorBufferInfo meshBufferInfo = {
        this->meshUniformBuffers_.buffers[i], 0, sizeof(MeshUniform)};

    VkDescriptorBufferInfo materialBufferInfo = {
        this->materialUniformBuffers_.buffers[i], 0, sizeof(MaterialUniform)};

    VkWriteDescriptorSet descriptorWrites[] = {
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            this->materialDescriptorSets_[i],          // dstSet
            0,                                         // dstBinding
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
            this->materialDescriptorSets_[i],  // dstSet
            1,                                 // dstBinding
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
            this->meshDescriptorSets_[i],      // dstSet
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
        ctx::device, ARRAYSIZE(descriptorWrites), descriptorWrites, 0, nullptr);
  }
}

void Billboard::draw(Window &window, GraphicsPipeline &pipeline) {
  auto commandBuffer = window.getCurrentCommandBuffer();

  auto i = window.getCurrentFrameIndex();

  vkCmdBindPipeline(
      commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

  vkCmdBindDescriptorSets(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline.pipelineLayout,
      2, // firstSet
      1,
      &meshDescriptorSets_[i],
      0,
      nullptr);

  vkCmdBindDescriptorSets(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline.pipelineLayout,
      1, // firstSet
      1,
      &materialDescriptorSets_[i],
      0,
      nullptr);

  vkCmdDraw(commandBuffer, 6, 1, 0, 0);
}

void Billboard::setPos(glm::vec3 pos) {
  this->meshUbo_.model[3][0] = pos.x;
  this->meshUbo_.model[3][1] = pos.y;
  this->meshUbo_.model[3][2] = pos.z;

  for (size_t i = 0; i < ARRAYSIZE(this->meshUniformBuffers_.buffers); i++) {
    memcpy(this->meshMappings_[i], &this->meshUbo_, sizeof(MeshUniform));
  }
}

void Billboard::setColor(glm::vec4 color) {
  this->materialUbo_.color = color;

  for (size_t i = 0; i < ARRAYSIZE(this->materialUniformBuffers_.buffers);
       i++) {
    memcpy(
        this->materialMappings_[i],
        &this->materialUbo_,
        sizeof(MaterialUniform));
  }
}

void Billboard::destroy() {
  VK_CHECK(vkDeviceWaitIdle(ctx::device));

  VK_CHECK(vkFreeDescriptorSets(
      ctx::device,
      *ctx::descriptorManager.getPool(DESC_MESH),
      ARRAYSIZE(this->meshDescriptorSets_),
      this->meshDescriptorSets_));

  VK_CHECK(vkFreeDescriptorSets(
      ctx::device,
      *ctx::descriptorManager.getPool(DESC_MATERIAL),
      ARRAYSIZE(this->materialDescriptorSets_),
      this->materialDescriptorSets_));

  for (size_t i = 0; i < ARRAYSIZE(this->meshUniformBuffers_.buffers); i++) {
    buffer::unmapMemory(this->meshUniformBuffers_.allocations[i]);
    buffer::destroy(
        this->meshUniformBuffers_.buffers[i],
        this->meshUniformBuffers_.allocations[i]);
  }

  for (size_t i = 0; i < ARRAYSIZE(this->materialUniformBuffers_.buffers);
       i++) {
    buffer::unmapMemory(this->materialUniformBuffers_.allocations[i]);
    buffer::destroy(
        this->materialUniformBuffers_.buffers[i],
        this->materialUniformBuffers_.allocations[i]);
  }
}
