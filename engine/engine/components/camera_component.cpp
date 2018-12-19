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
    renderer::Window &window, const TransformComponent &transform) {
  auto i = window.getCurrentFrameIndex();

  m_cameraUniform.proj = glm::perspective(
      glm::radians(m_fov),
      static_cast<float>(window.getWidth()) /
          static_cast<float>(window.getHeight()),
      m_near,
      m_far);

  // See: https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/
  glm::mat4 correction(
      {1.0, 0.0, 0.0, 0.0},
      {0.0, -1.0, 0.0, 0.0},
      {0.0, 0.0, 0.5, 0.5},
      {0.0, 0.0, 0.0, 1.0});

  m_cameraUniform.proj = correction * m_cameraUniform.proj;

  m_cameraUniform.view = glm::mat4(1.0);
  m_cameraUniform.view =
      glm::mat4_cast(transform.rotation) *
      glm::translate(m_cameraUniform.view, transform.position * -1.0f);
  m_cameraUniform.view = glm::scale(m_cameraUniform.view, transform.scale);

  m_cameraUniform.pos = glm::vec4(transform.position, 1.0);

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