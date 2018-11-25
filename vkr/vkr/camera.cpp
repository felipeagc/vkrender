#include "camera.hpp"
#include "context.hpp"
#include "pipeline.hpp"
#include "util.hpp"

using namespace vkr;

Camera::Camera() {
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    m_uniformBuffers.buffers[i] = VK_NULL_HANDLE;
    m_uniformBuffers.allocations[i] = VK_NULL_HANDLE;
    m_descriptorSets[i] = VK_NULL_HANDLE;
  }
}

Camera::Camera(glm::vec3 position, glm::quat rotation) {
  this->setPos(position);
  this->setRot(rotation);

  auto [descriptorPool, descriptorSetLayout] =
      ctx().m_descriptorManager[DESC_CAMERA];

  assert(descriptorPool != nullptr && descriptorSetLayout != nullptr);

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    buffer::createUniformBuffer(
        sizeof(CameraUniform),
        &m_uniformBuffers.buffers[i],
        &m_uniformBuffers.allocations[i]);

    buffer::mapMemory(m_uniformBuffers.allocations[i], &m_mappings[i]);

    VkDescriptorSetAllocateInfo allocateInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        nullptr,
        *descriptorPool,
        1,
        descriptorSetLayout,
    };

    vkAllocateDescriptorSets(
        ctx().m_device, &allocateInfo, &m_descriptorSets[i]);

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

    vkUpdateDescriptorSets(ctx().m_device, 1, &descriptorWrite, 0, nullptr);
  }
}

Camera::~Camera() {
  VK_CHECK(vkDeviceWaitIdle(ctx().m_device));

  if (m_uniformBuffers.buffers[0] != VK_NULL_HANDLE) {
    for (size_t i = 0; i < ARRAYSIZE(m_uniformBuffers.buffers); i++) {
      buffer::unmapMemory(m_uniformBuffers.allocations[i]);
      buffer::destroy(
          m_uniformBuffers.buffers[i], m_uniformBuffers.allocations[i]);
    }
  }

  if (m_descriptorSets[0] != VK_NULL_HANDLE) {
    auto descriptorPool = ctx().m_descriptorManager.getPool(DESC_CAMERA);

    assert(descriptorPool != nullptr);

    vkFreeDescriptorSets(
        ctx().m_device,
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

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    m_uniformBuffers.buffers[i] = rhs.m_uniformBuffers.buffers[i];
    m_uniformBuffers.allocations[i] = rhs.m_uniformBuffers.allocations[i];
    m_mappings[i] = rhs.m_mappings[i];
    m_descriptorSets[i] = rhs.m_descriptorSets[i];
  }

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    rhs.m_uniformBuffers.buffers[i] = VK_NULL_HANDLE;
    rhs.m_uniformBuffers.allocations[i] = VK_NULL_HANDLE;
    rhs.m_mappings[i] = VK_NULL_HANDLE;
    rhs.m_descriptorSets[i] = VK_NULL_HANDLE;
  }
}

Camera &Camera::operator=(Camera &&rhs) {
  if (this != &rhs) {
    // Free old stuff
    VK_CHECK(vkDeviceWaitIdle(ctx().m_device));

    if (m_uniformBuffers.buffers[0] != VK_NULL_HANDLE) {
      for (size_t i = 0; i < ARRAYSIZE(m_uniformBuffers.buffers); i++) {
        buffer::unmapMemory(m_uniformBuffers.allocations[i]);
        buffer::destroy(
            m_uniformBuffers.buffers[i], m_uniformBuffers.allocations[i]);
      }
    }

    if (m_descriptorSets[0] != VK_NULL_HANDLE) {
      auto descriptorPool = ctx().m_descriptorManager.getPool(DESC_CAMERA);

      assert(descriptorPool != nullptr);

      vkFreeDescriptorSets(
          ctx().m_device,
          *descriptorPool,
          ARRAYSIZE(m_descriptorSets),
          m_descriptorSets);
    }
  }

  m_cameraUniform = rhs.m_cameraUniform;
  m_fov = rhs.m_fov;
  m_near = rhs.m_near;
  m_far = rhs.m_far;

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    m_uniformBuffers.buffers[i] = rhs.m_uniformBuffers.buffers[i];
    m_uniformBuffers.allocations[i] = rhs.m_uniformBuffers.allocations[i];
    m_mappings[i] = rhs.m_mappings[i];
    m_descriptorSets[i] = rhs.m_descriptorSets[i];
  }

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    rhs.m_uniformBuffers.buffers[i] = VK_NULL_HANDLE;
    rhs.m_uniformBuffers.allocations[i] = VK_NULL_HANDLE;
    rhs.m_mappings[i] = VK_NULL_HANDLE;
    rhs.m_descriptorSets[i] = VK_NULL_HANDLE;
  }

  return *this;
}

void Camera::setPos(glm::vec3 pos) {
  m_cameraUniform.view[3][0] = pos.x;
  m_cameraUniform.view[3][1] = pos.y;
  m_cameraUniform.view[3][2] = pos.z;
}

glm::vec3 Camera::getPos() const {
  return glm::vec3(
      m_cameraUniform.view[3][0],
      m_cameraUniform.view[3][1],
      m_cameraUniform.view[3][2]);
}

void Camera::translate(glm::vec3 translation) {
  m_cameraUniform.view = glm::translate(m_cameraUniform.view, translation);
}

void Camera::setRot(glm::quat rot) {
  glm::quat currentRot = glm::quat_cast(m_cameraUniform.view);
  m_cameraUniform.view =
      glm::mat4_cast(glm::inverse(currentRot)) * m_cameraUniform.view;
  m_cameraUniform.view = glm::mat4_cast(rot) * m_cameraUniform.view;
}

glm::quat Camera::getRot() const {
  glm::quat currentRot = glm::quat_cast(m_cameraUniform.view);
  return currentRot;
}

void Camera::rotate(glm::quat rot) {
  m_cameraUniform.view = glm::mat4_cast(rot) * m_cameraUniform.view;
}

void Camera::setFov(float fov) { m_fov = fov; }

float Camera::getFov() const { return m_fov; }

void Camera::lookAt(glm::vec3 point) {
  m_cameraUniform.view = glm::lookAt(this->getPos(), point, {0.0, -1.0, 0.0});
}

void Camera::update(Window &window) {
  auto i = window.getCurrentFrameIndex();

  m_cameraUniform.proj = glm::perspective(
      glm::radians(m_fov),
      static_cast<float>(window.getWidth()) /
          static_cast<float>(window.getHeight()),
      m_near,
      m_far);

  memcpy(m_mappings[i], &m_cameraUniform, sizeof(CameraUniform));
}

void Camera::bind(Window &window, GraphicsPipeline &pipeline) {
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
