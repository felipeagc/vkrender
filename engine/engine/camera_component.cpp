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
    m_uniformBuffers[i] =
        renderer::Buffer{renderer::BufferType::eUniform, sizeof(CameraUniform)};

    m_uniformBuffers[i].mapMemory(&m_mappings[i]);

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
        m_uniformBuffers[i].getHandle(),
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

  for (size_t i = 0; i < ARRAYSIZE(m_uniformBuffers); i++) {
    m_uniformBuffers[i].unmapMemory();
    m_uniformBuffers[i].destroy();
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

  glm::mat4 view = transform;
  view[3][0] *= -1.0f;
  view[3][1] *= -1.0f;
  view[3][2] *= -1.0f;

  m_cameraUniform.view =
      glm::lookAt(glm::vec3(0.0f), m_front, m_up) * view;

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
