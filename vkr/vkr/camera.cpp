#include "camera.hpp"
#include "commandbuffer.hpp"
#include "context.hpp"
#include "graphics_pipeline.hpp"

using namespace vkr;

Camera::Camera(glm::vec3 position, glm::quat rotation) {
  this->setPos(position);
  this->setRot(rotation);

  auto [descriptorPool, descriptorSetLayout] =
      Context::getDescriptorManager()[DESC_CAMERA];

  assert(descriptorPool != nullptr && descriptorSetLayout != nullptr);

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    this->uniformBuffers_[i] = Buffer{
        sizeof(CameraUniform),
        vkr::BufferUsageFlagBits::eUniformBuffer,
        vkr::MemoryUsageFlagBits::eCpuToGpu,
        vkr::MemoryPropertyFlagBits::eHostCoherent,
    };

    this->uniformBuffers_[i].mapMemory(&this->mappings_[i]);

    DescriptorBufferInfo bufferInfo{
        this->uniformBuffers_[i].getHandle(),
        0,
        sizeof(CameraUniform),
    };

    this->descriptorSets_[i] =
        descriptorPool->allocateDescriptorSets(1, *descriptorSetLayout)[0];

    Context::getDevice().updateDescriptorSets(
        {vk::WriteDescriptorSet{
            this->descriptorSets_[i],             // dstSet
            0,                                   // dstBinding
            0,                                   // dstArrayElement
            1,                                   // descriptorCount
            vkr::DescriptorType::eUniformBuffer, // descriptorType
            nullptr,                             // pImageInfo
            &bufferInfo,                         // pBufferInfo
            nullptr,                             // pTexelBufferView
        }},
        {});
  }
}

Camera::~Camera() {
  for (auto &uniformBuffer : uniformBuffers_) {
    uniformBuffer.unmapMemory();
    uniformBuffer.destroy();
  }

  auto descriptorPool = Context::getDescriptorManager().getPool(DESC_CAMERA);

  assert(descriptorPool != nullptr);

  Context::getDevice().freeDescriptorSets(
      *descriptorPool, this->descriptorSets_);
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

  window.getCurrentCommandBuffer().bindDescriptorSets(
      vkr::PipelineBindPoint::eGraphics,
      pipeline.getLayout(),
      0,                       // firstIndex
      this->descriptorSets_[i], // pDescriptorSets
      {});
}
