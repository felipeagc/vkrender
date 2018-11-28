#include "camera_component.hpp"
#include <renderer/context.hpp>
#include <renderer/pipeline.hpp>
#include <renderer/util.hpp>

using namespace engine;

CameraComponent::CameraComponent(float fov) : m_fov(fov) {
  auto [descriptorPool, descriptorSetLayout] =
      renderer::ctx().m_descriptorManager[renderer::DESC_CAMERA];

  assert(descriptorPool != nullptr && descriptorSetLayout != nullptr);

  for (int i = 0; i < renderer::MAX_FRAMES_IN_FLIGHT; i++) {
    renderer::buffer::createUniformBuffer(
        sizeof(CameraUniform),
        &m_uniformBuffers.buffers[i],
        &m_uniformBuffers.allocations[i]);

    renderer::buffer::mapMemory(
        m_uniformBuffers.allocations[i], &m_mappings[i]);

    VkDescriptorSetAllocateInfo allocateInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        nullptr,
        *descriptorPool,
        1,
        descriptorSetLayout,
    };

    vkAllocateDescriptorSets(
        renderer::ctx().m_device, &allocateInfo, &m_descriptorSets[i]);

    VkDescriptorBufferInfo bufferInfo{
        m_uniformBuffers.buffers[i],
        0,
        sizeof(CameraUniform),
    };

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

CameraComponent::~CameraComponent() {
  VK_CHECK(vkDeviceWaitIdle(renderer::ctx().m_device));

  for (size_t i = 0; i < ARRAYSIZE(m_uniformBuffers.buffers); i++) {
    renderer::buffer::unmapMemory(m_uniformBuffers.allocations[i]);
    renderer::buffer::destroy(
        m_uniformBuffers.buffers[i], m_uniformBuffers.allocations[i]);
  }

  auto descriptorPool =
      renderer::ctx().m_descriptorManager.getPool(renderer::DESC_CAMERA);

  assert(descriptorPool != nullptr);

  vkFreeDescriptorSets(
      renderer::ctx().m_device,
      *descriptorPool,
      ARRAYSIZE(m_descriptorSets),
      m_descriptorSets);
}

void CameraComponent::update(
    renderer::Window &window, const glm::mat4 &transform) {
  auto i = window.getCurrentFrameIndex();

  m_cameraUniform.proj = glm::perspective(
      glm::radians(m_fov),
      static_cast<float>(window.getWidth()) /
          static_cast<float>(window.getHeight()),
      m_near,
      m_far);

  m_cameraUniform.view = transform;

  memcpy(m_mappings[i], &m_cameraUniform, sizeof(CameraUniform));
}

void CameraComponent::bind(
    renderer::Window &window, renderer::GraphicsPipeline &pipeline) {
  auto i = window.getCurrentFrameIndex();
  VkCommandBuffer commandBuffer = window.getCurrentCommandBuffer();

  vkCmdBindDescriptorSets(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline.m_pipelineLayout,
      0,
      1,
      &m_descriptorSets[i],
      0,
      nullptr);
}
