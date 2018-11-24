#include "camera.hpp"
#include "context.hpp"
#include "pipeline.hpp"
#include "util.hpp"

using namespace vkr;

Camera::Camera(glm::vec3 position, glm::quat rotation) {
  this->setPos(position);
  this->setRot(rotation);

  auto [descriptorPool, descriptorSetLayout] =
      ctx::descriptorManager[DESC_CAMERA];

  assert(descriptorPool != nullptr && descriptorSetLayout != nullptr);

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    buffer::createUniformBuffer(
        sizeof(CameraUniform),
        &this->m_uniformBuffers.buffers[i],
        &this->m_uniformBuffers.allocations[i]);

    buffer::mapMemory(
        this->m_uniformBuffers.allocations[i], &this->m_mappings[i]);

    VkDescriptorSetAllocateInfo allocateInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        nullptr,
        *descriptorPool,
        1,
        descriptorSetLayout,
    };

    vkAllocateDescriptorSets(
        ctx::device, &allocateInfo, &this->m_descriptorSets[i]);

    VkDescriptorBufferInfo bufferInfo{
        this->m_uniformBuffers.buffers[i],
        0,
        sizeof(CameraUniform),
    };

    VkWriteDescriptorSet descriptorWrite = {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        nullptr,
        this->m_descriptorSets[i],          // dstSet
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

void Camera::destroy() {
  for (size_t i = 0; i < ARRAYSIZE(this->m_uniformBuffers.buffers); i++) {
    buffer::unmapMemory(this->m_uniformBuffers.allocations[i]);
    buffer::destroy(
        this->m_uniformBuffers.buffers[i], this->m_uniformBuffers.allocations[i]);
  }

  auto descriptorPool = ctx::descriptorManager.getPool(DESC_CAMERA);

  assert(descriptorPool != nullptr);

  vkFreeDescriptorSets(
      ctx::device,
      *descriptorPool,
      this->m_descriptorSets.size(),
      this->m_descriptorSets.data());
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

void Camera::setFov(float fov) { this->m_fov = fov; }

float Camera::getFov() const { return this->m_fov; }

void Camera::lookAt(glm::vec3 point) {
  m_cameraUniform.view = glm::lookAt(this->getPos(), point, {0.0, -1.0, 0.0});
}

void Camera::update(Window &window) {
  auto i = window.getCurrentFrameIndex();

  m_cameraUniform.proj = glm::perspective(
      glm::radians(this->m_fov),
      static_cast<float>(window.getWidth()) /
          static_cast<float>(window.getHeight()),
      this->m_near,
      this->m_far);

  memcpy(this->m_mappings[i], &this->m_cameraUniform, sizeof(CameraUniform));
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
      &this->m_descriptorSets[i],
      0,
      nullptr);
}
