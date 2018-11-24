#include "billboard.hpp"
#include "context.hpp"
#include "pipeline.hpp"
#include "util.hpp"

using namespace vkr;

Billboard::Billboard(
    const Texture &texture, glm::vec3 pos, glm::vec3 scale, glm::vec4 color)
    : m_texture(texture) {

  // Initialize mesh UBO struct
  glm::mat4 model(1.0f);
  model = glm::translate(model, pos);
  model = glm::scale(model, scale);
  this->m_meshUbo.model = model;

  // Initialize material UBO struct
  this->m_materialUbo.color = color;

  // Create mesh uniform buffers
  for (size_t i = 0; i < ARRAYSIZE(this->m_meshUniformBuffers.buffers); i++) {
    buffer::createUniformBuffer(
        sizeof(MeshUniform),
        &this->m_meshUniformBuffers.buffers[i],
        &this->m_meshUniformBuffers.allocations[i]);

    buffer::mapMemory(
        this->m_meshUniformBuffers.allocations[i], &this->m_meshMappings[i]);
    memcpy(this->m_meshMappings[i], &this->m_meshUbo, sizeof(MeshUniform));
  }

  // Create material uniform buffers
  for (size_t i = 0; i < ARRAYSIZE(this->m_materialUniformBuffers.buffers);
       i++) {
    buffer::createUniformBuffer(
        sizeof(MaterialUniform),
        &this->m_materialUniformBuffers.buffers[i],
        &this->m_materialUniformBuffers.allocations[i]);

    buffer::mapMemory(
        this->m_materialUniformBuffers.allocations[i],
        &this->m_materialMappings[i]);
    memcpy(
        this->m_materialMappings[i],
        &this->m_materialUbo,
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

    for (size_t i = 0; i < ARRAYSIZE(this->m_meshDescriptorSets); i++) {
      VK_CHECK(vkAllocateDescriptorSets(
          ctx::device, &allocateInfo, &this->m_meshDescriptorSets[i]));
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

    for (size_t i = 0; i < ARRAYSIZE(this->m_materialDescriptorSets); i++) {
      VK_CHECK(vkAllocateDescriptorSets(
          ctx::device, &allocateInfo, &this->m_materialDescriptorSets[i]));
    }
  }

  // Update descriptor sets
  for (size_t i = 0; i < ARRAYSIZE(this->m_materialDescriptorSets); i++) {
    auto albedoDescriptorInfo = this->m_texture.getDescriptorInfo();

    VkDescriptorBufferInfo meshBufferInfo = {
        this->m_meshUniformBuffers.buffers[i], 0, sizeof(MeshUniform)};

    VkDescriptorBufferInfo materialBufferInfo = {
        this->m_materialUniformBuffers.buffers[i], 0, sizeof(MaterialUniform)};

    VkWriteDescriptorSet descriptorWrites[] = {
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            this->m_materialDescriptorSets[i],          // dstSet
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
            this->m_materialDescriptorSets[i],  // dstSet
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
            this->m_meshDescriptorSets[i],      // dstSet
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

void Billboard::setPos(glm::vec3 pos) {
  this->m_meshUbo.model[3][0] = pos.x;
  this->m_meshUbo.model[3][1] = pos.y;
  this->m_meshUbo.model[3][2] = pos.z;

  for (size_t i = 0; i < ARRAYSIZE(this->m_meshUniformBuffers.buffers); i++) {
    memcpy(this->m_meshMappings[i], &this->m_meshUbo, sizeof(MeshUniform));
  }
}

void Billboard::setColor(glm::vec4 color) {
  this->m_materialUbo.color = color;

  for (size_t i = 0; i < ARRAYSIZE(this->m_materialUniformBuffers.buffers);
       i++) {
    memcpy(
        this->m_materialMappings[i],
        &this->m_materialUbo,
        sizeof(MaterialUniform));
  }
}

void Billboard::destroy() {
  VK_CHECK(vkDeviceWaitIdle(ctx::device));

  VK_CHECK(vkFreeDescriptorSets(
      ctx::device,
      *ctx::descriptorManager.getPool(DESC_MESH),
      ARRAYSIZE(this->m_meshDescriptorSets),
      this->m_meshDescriptorSets));

  VK_CHECK(vkFreeDescriptorSets(
      ctx::device,
      *ctx::descriptorManager.getPool(DESC_MATERIAL),
      ARRAYSIZE(this->m_materialDescriptorSets),
      this->m_materialDescriptorSets));

  for (size_t i = 0; i < ARRAYSIZE(this->m_meshUniformBuffers.buffers); i++) {
    buffer::unmapMemory(this->m_meshUniformBuffers.allocations[i]);
    buffer::destroy(
        this->m_meshUniformBuffers.buffers[i],
        this->m_meshUniformBuffers.allocations[i]);
  }

  for (size_t i = 0; i < ARRAYSIZE(this->m_materialUniformBuffers.buffers);
       i++) {
    buffer::unmapMemory(this->m_materialUniformBuffers.allocations[i]);
    buffer::destroy(
        this->m_materialUniformBuffers.buffers[i],
        this->m_materialUniformBuffers.allocations[i]);
  }
}
