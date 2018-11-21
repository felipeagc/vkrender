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
    buffer::makeUniformBuffer(
        sizeof(CameraUniform),
        &this->uniformBuffers_.buffers[i],
        &this->uniformBuffers_.allocations[i]);

    buffer::mapMemory(
        this->uniformBuffers_.allocations[i], &this->mappings_[i]);

    VkDescriptorSetAllocateInfo allocateInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        nullptr,
        *descriptorPool,
        1,
        descriptorSetLayout,
    };

    vkAllocateDescriptorSets(
        ctx::device, &allocateInfo, &this->descriptorSets_[i]);

    VkDescriptorBufferInfo bufferInfo{
        this->uniformBuffers_.buffers[i],
        0,
        sizeof(CameraUniform),
    };

    VkWriteDescriptorSet descriptorWrite = {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        nullptr,
        this->descriptorSets_[i],          // dstSet
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
  for (size_t i = 0; i < ARRAYSIZE(this->uniformBuffers_.buffers); i++) {
    buffer::unmapMemory(this->uniformBuffers_.allocations[i]);
    buffer::destroy(
        this->uniformBuffers_.buffers[i], this->uniformBuffers_.allocations[i]);
  }

  auto descriptorPool = ctx::descriptorManager.getPool(DESC_CAMERA);

  assert(descriptorPool != nullptr);

  vkFreeDescriptorSets(
      ctx::device,
      *descriptorPool,
      this->descriptorSets_.size(),
      this->descriptorSets_.data());
}

void Camera::setPos(glm::vec3 pos) {
  cameraUniform_.view[3][0] = pos.x;
  cameraUniform_.view[3][1] = pos.y;
  cameraUniform_.view[3][2] = pos.z;
}

glm::vec3 Camera::getPos() const {
  return glm::vec3(
      cameraUniform_.view[3][0],
      cameraUniform_.view[3][1],
      cameraUniform_.view[3][2]);
}

void Camera::translate(glm::vec3 translation) {
  cameraUniform_.view = glm::translate(cameraUniform_.view, translation);
}

void Camera::setRot(glm::quat rot) {
  glm::quat currentRot = glm::quat_cast(cameraUniform_.view);
  cameraUniform_.view =
      glm::mat4_cast(glm::inverse(currentRot)) * cameraUniform_.view;
  cameraUniform_.view = glm::mat4_cast(rot) * cameraUniform_.view;
}

glm::quat Camera::getRot() const {
  glm::quat currentRot = glm::quat_cast(cameraUniform_.view);
  return currentRot;
}

void Camera::rotate(glm::quat rot) {
  cameraUniform_.view = glm::mat4_cast(rot) * cameraUniform_.view;
}

void Camera::setFov(float fov) { this->fov_ = fov; }

float Camera::getFov() const { return this->fov_; }

void Camera::lookAt(glm::vec3 point) {
  cameraUniform_.view = glm::lookAt(this->getPos(), point, {0.0, -1.0, 0.0});
}

void Camera::update(Window &window) {
  auto i = window.getCurrentFrameIndex();

  cameraUniform_.proj = glm::perspective(
      glm::radians(this->fov_),
      static_cast<float>(window.getWidth()) /
          static_cast<float>(window.getHeight()),
      this->near_,
      this->far_);

  memcpy(this->mappings_[i], &this->cameraUniform_, sizeof(CameraUniform));
}

void Camera::bind(Window &window, GraphicsPipeline &pipeline) {
  auto i = window.getCurrentFrameIndex();
  VkCommandBuffer commandBuffer = window.getCurrentCommandBuffer();

  vkCmdBindDescriptorSets(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline.pipelineLayout,
      0,
      1,
      &this->descriptorSets_[i],
      0,
      nullptr);
}
