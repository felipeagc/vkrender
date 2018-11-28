#include "camera.hpp"
#include <renderer/context.hpp>
#include <renderer/pipeline.hpp>
#include <renderer/util.hpp>

using namespace engine;

Camera::Camera() {
  for (int i = 0; i < renderer::MAX_FRAMES_IN_FLIGHT; i++) {
    m_uniformBuffers.buffers[i] = VK_NULL_HANDLE;
    m_uniformBuffers.allocations[i] = VK_NULL_HANDLE;
    m_descriptorSets[i] = VK_NULL_HANDLE;
  }
}

Camera::Camera(glm::vec3 position, glm::quat rotation)
    : m_position(position), m_rotation(rotation) {
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

Camera::~Camera() {
  if (*this) {
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
}

Camera::Camera(Camera &&rhs) {
  m_cameraUniform = rhs.m_cameraUniform;
  m_fov = rhs.m_fov;
  m_near = rhs.m_near;
  m_far = rhs.m_far;
  m_position = rhs.m_position;
  m_rotation = rhs.m_rotation;

  for (int i = 0; i < renderer::MAX_FRAMES_IN_FLIGHT; i++) {
    m_uniformBuffers.buffers[i] = rhs.m_uniformBuffers.buffers[i];
    m_uniformBuffers.allocations[i] = rhs.m_uniformBuffers.allocations[i];
    m_mappings[i] = rhs.m_mappings[i];
    m_descriptorSets[i] = rhs.m_descriptorSets[i];
  }

  for (int i = 0; i < renderer::MAX_FRAMES_IN_FLIGHT; i++) {
    rhs.m_uniformBuffers.buffers[i] = VK_NULL_HANDLE;
    rhs.m_uniformBuffers.allocations[i] = VK_NULL_HANDLE;
    rhs.m_mappings[i] = VK_NULL_HANDLE;
    rhs.m_descriptorSets[i] = VK_NULL_HANDLE;
  }
}

Camera &Camera::operator=(Camera &&rhs) {
  if (this != &rhs) {
    if (*this) {
      // Free old stuff
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
  }

  m_cameraUniform = rhs.m_cameraUniform;
  m_fov = rhs.m_fov;
  m_near = rhs.m_near;
  m_far = rhs.m_far;
  m_position = rhs.m_position;
  m_rotation = rhs.m_rotation;

  for (int i = 0; i < renderer::MAX_FRAMES_IN_FLIGHT; i++) {
    m_uniformBuffers.buffers[i] = rhs.m_uniformBuffers.buffers[i];
    m_uniformBuffers.allocations[i] = rhs.m_uniformBuffers.allocations[i];
    m_mappings[i] = rhs.m_mappings[i];
    m_descriptorSets[i] = rhs.m_descriptorSets[i];
  }

  for (int i = 0; i < renderer::MAX_FRAMES_IN_FLIGHT; i++) {
    rhs.m_uniformBuffers.buffers[i] = VK_NULL_HANDLE;
    rhs.m_uniformBuffers.allocations[i] = VK_NULL_HANDLE;
    rhs.m_mappings[i] = VK_NULL_HANDLE;
    rhs.m_descriptorSets[i] = VK_NULL_HANDLE;
  }

  return *this;
}

Camera::operator bool() const {
  return (m_descriptorSets[0] != VK_NULL_HANDLE);
}

void Camera::lookAt(glm::vec3 point, glm::vec3 up) {
  m_rotation =
      glm::conjugate(glm::quatLookAt(glm::normalize(m_position - point), up));
}

void Camera::update(renderer::Window &window) {
  auto i = window.getCurrentFrameIndex();

  m_cameraUniform.proj = glm::perspective(
      glm::radians(m_fov),
      static_cast<float>(window.getWidth()) /
          static_cast<float>(window.getHeight()),
      m_near,
      m_far);

  m_cameraUniform.view = glm::mat4(1.0f);
  m_cameraUniform.view = glm::toMat4(m_rotation) * m_cameraUniform.view;
  m_cameraUniform.view = glm::translate(m_cameraUniform.view, m_position);

  memcpy(m_mappings[i], &m_cameraUniform, sizeof(CameraUniform));
}

void Camera::bind(
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
