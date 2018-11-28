#include "billboard.hpp"
#include <fstl/logging.hpp>
#include <renderer/context.hpp>
#include <renderer/util.hpp>

using namespace engine;

Billboard::Billboard() {
  for (int i = 0; i < renderer::MAX_FRAMES_IN_FLIGHT; i++) {
    m_meshDescriptorSets[i] = VK_NULL_HANDLE;
    m_materialDescriptorSets[i] = VK_NULL_HANDLE;

    m_meshUniformBuffers.buffers[i] = VK_NULL_HANDLE;
    m_meshUniformBuffers.allocations[i] = VK_NULL_HANDLE;

    m_materialUniformBuffers.buffers[i] = VK_NULL_HANDLE;
    m_materialUniformBuffers.allocations[i] = VK_NULL_HANDLE;
  }
}

Billboard::Billboard(
    const renderer::Texture &texture,
    glm::vec3 pos,
    glm::vec3 scale,
    glm::vec4 color)
    : m_texture(texture) {
  // Initialize mesh UBO struct
  glm::mat4 model(1.0f);
  model = glm::translate(model, pos);
  model = glm::scale(model, scale);
  m_meshUbo.model = model;

  // Initialize material UBO struct
  m_materialUbo.color = color;

  // Create mesh uniform buffers
  for (size_t i = 0; i < ARRAYSIZE(m_meshUniformBuffers.buffers); i++) {
    renderer::buffer::createUniformBuffer(
        sizeof(MeshUniform),
        &m_meshUniformBuffers.buffers[i],
        &m_meshUniformBuffers.allocations[i]);

    renderer::buffer::mapMemory(
        m_meshUniformBuffers.allocations[i], &m_meshMappings[i]);
    memcpy(m_meshMappings[i], &m_meshUbo, sizeof(MeshUniform));
  }

  // Create material uniform buffers
  for (size_t i = 0; i < ARRAYSIZE(m_materialUniformBuffers.buffers); i++) {
    renderer::buffer::createUniformBuffer(
        sizeof(MaterialUniform),
        &m_materialUniformBuffers.buffers[i],
        &m_materialUniformBuffers.allocations[i]);

    renderer::buffer::mapMemory(
        m_materialUniformBuffers.allocations[i], &m_materialMappings[i]);
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
        m_meshUniformBuffers.buffers[i], 0, sizeof(MeshUniform)};

    VkDescriptorBufferInfo materialBufferInfo = {
        m_materialUniformBuffers.buffers[i], 0, sizeof(MaterialUniform)};

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
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            m_materialDescriptorSets[i],       // dstSet
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

Billboard::~Billboard() {
  if (*this) {
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

    for (size_t i = 0; i < ARRAYSIZE(m_meshUniformBuffers.buffers); i++) {
      renderer::buffer::unmapMemory(m_meshUniformBuffers.allocations[i]);
      renderer::buffer::destroy(
          m_meshUniformBuffers.buffers[i], m_meshUniformBuffers.allocations[i]);
    }

    for (size_t i = 0; i < ARRAYSIZE(m_materialUniformBuffers.buffers); i++) {
      renderer::buffer::unmapMemory(m_materialUniformBuffers.allocations[i]);
      renderer::buffer::destroy(
          m_materialUniformBuffers.buffers[i],
          m_materialUniformBuffers.allocations[i]);
    }
  }
}

Billboard::Billboard(Billboard &&rhs) {
  m_texture = rhs.m_texture;
  m_meshUbo = rhs.m_meshUbo;
  m_materialUbo = rhs.m_materialUbo;

  for (int i = 0; i < renderer::MAX_FRAMES_IN_FLIGHT; i++) {
    m_meshUniformBuffers.buffers[i] = rhs.m_meshUniformBuffers.buffers[i];
    m_meshUniformBuffers.allocations[i] =
        rhs.m_meshUniformBuffers.allocations[i];
    m_meshMappings[i] = rhs.m_meshMappings[i];
    m_meshDescriptorSets[i] = rhs.m_meshDescriptorSets[i];

    m_materialUniformBuffers.buffers[i] =
        rhs.m_materialUniformBuffers.buffers[i];
    m_materialUniformBuffers.allocations[i] =
        rhs.m_materialUniformBuffers.allocations[i];
    m_materialMappings[i] = rhs.m_materialMappings[i];
    m_materialDescriptorSets[i] = rhs.m_materialDescriptorSets[i];
  }

  for (int i = 0; i < renderer::MAX_FRAMES_IN_FLIGHT; i++) {
    rhs.m_meshUniformBuffers.buffers[i] = VK_NULL_HANDLE;
    rhs.m_meshUniformBuffers.allocations[i] = VK_NULL_HANDLE;
    rhs.m_meshMappings[i] = VK_NULL_HANDLE;
    rhs.m_meshDescriptorSets[i] = VK_NULL_HANDLE;

    rhs.m_materialUniformBuffers.buffers[i] = VK_NULL_HANDLE;
    rhs.m_materialUniformBuffers.allocations[i] = VK_NULL_HANDLE;
    rhs.m_materialMappings[i] = VK_NULL_HANDLE;
    rhs.m_materialDescriptorSets[i] = VK_NULL_HANDLE;
  }
}

Billboard &Billboard::operator=(Billboard &&rhs) {
  if (this != &rhs) {
    if (*this) {
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

      for (size_t i = 0; i < ARRAYSIZE(m_meshUniformBuffers.buffers); i++) {
        renderer::buffer::unmapMemory(m_meshUniformBuffers.allocations[i]);
        renderer::buffer::destroy(
            m_meshUniformBuffers.buffers[i],
            m_meshUniformBuffers.allocations[i]);
      }

      for (size_t i = 0; i < ARRAYSIZE(m_materialUniformBuffers.buffers); i++) {
        renderer::buffer::unmapMemory(m_materialUniformBuffers.allocations[i]);
        renderer::buffer::destroy(
            m_materialUniformBuffers.buffers[i],
            m_materialUniformBuffers.allocations[i]);
      }
    }
  }

  m_texture = rhs.m_texture;
  m_meshUbo = rhs.m_meshUbo;
  m_materialUbo = rhs.m_materialUbo;

  for (int i = 0; i < renderer::MAX_FRAMES_IN_FLIGHT; i++) {
    m_meshUniformBuffers.buffers[i] = rhs.m_meshUniformBuffers.buffers[i];
    m_meshUniformBuffers.allocations[i] =
        rhs.m_meshUniformBuffers.allocations[i];
    m_meshMappings[i] = rhs.m_meshMappings[i];
    m_meshDescriptorSets[i] = rhs.m_meshDescriptorSets[i];

    m_materialUniformBuffers.buffers[i] =
        rhs.m_materialUniformBuffers.buffers[i];
    m_materialUniformBuffers.allocations[i] =
        rhs.m_materialUniformBuffers.allocations[i];
    m_materialMappings[i] = rhs.m_materialMappings[i];
    m_materialDescriptorSets[i] = rhs.m_materialDescriptorSets[i];
  }

  for (int i = 0; i < renderer::MAX_FRAMES_IN_FLIGHT; i++) {
    rhs.m_meshUniformBuffers.buffers[i] = VK_NULL_HANDLE;
    rhs.m_meshUniformBuffers.allocations[i] = VK_NULL_HANDLE;
    rhs.m_meshMappings[i] = VK_NULL_HANDLE;
    rhs.m_meshDescriptorSets[i] = VK_NULL_HANDLE;

    rhs.m_materialUniformBuffers.buffers[i] = VK_NULL_HANDLE;
    rhs.m_materialUniformBuffers.allocations[i] = VK_NULL_HANDLE;
    rhs.m_materialMappings[i] = VK_NULL_HANDLE;
    rhs.m_materialDescriptorSets[i] = VK_NULL_HANDLE;
  }

  return *this;
}

Billboard::operator bool() const {
  return (m_meshDescriptorSets[0] != VK_NULL_HANDLE);
}

void Billboard::draw(
    renderer::Window &window, renderer::GraphicsPipeline &pipeline) {
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
  m_meshUbo.model[3][0] = pos.x;
  m_meshUbo.model[3][1] = pos.y;
  m_meshUbo.model[3][2] = pos.z;

  for (size_t i = 0; i < ARRAYSIZE(m_meshUniformBuffers.buffers); i++) {
    memcpy(m_meshMappings[i], &m_meshUbo, sizeof(MeshUniform));
  }
}

void Billboard::setColor(glm::vec4 color) {
  m_materialUbo.color = color;

  for (size_t i = 0; i < ARRAYSIZE(m_materialUniformBuffers.buffers); i++) {
    memcpy(m_materialMappings[i], &m_materialUbo, sizeof(MaterialUniform));
  }
}
