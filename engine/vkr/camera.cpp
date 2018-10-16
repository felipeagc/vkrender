#include "camera.hpp"
#include "commandbuffer.hpp"
#include "context.hpp"

using namespace vkr;

Camera::Camera(glm::vec3 position)
    : uniformBuffer(
          sizeof(CameraUniform),
          vkr::BufferUsageFlagBits::eUniformBuffer,
          vkr::MemoryUsageFlagBits::eCpuToGpu,
          vkr::MemoryPropertyFlagBits::eHostCoherent),
      pos(position) {
  uniformBuffer.mapMemory(&this->mapping);

  this->bufferInfo = DescriptorBufferInfo{
      uniformBuffer.getVkBuffer(),
      0,
      sizeof(CameraUniform),
  };

  auto [descriptorPool, descriptorSetLayout] =
      Context::getDescriptorManager()[DESC_CAMERA];

  assert(descriptorPool != nullptr && descriptorSetLayout != nullptr);

  this->descriptorSet =
      descriptorPool->allocateDescriptorSets(1, *descriptorSetLayout)[0];

  vkr::Context::getDevice().updateDescriptorSets(
      {vk::WriteDescriptorSet{
          descriptorSet,                       // dstSet
          0,                                   // dstBinding
          0,                                   // dstArrayElement
          1,                                   // descriptorCount
          vkr::DescriptorType::eUniformBuffer, // descriptorType
          nullptr,                             // pImageInfo
          &this->bufferInfo,                   // pBufferInfo
          nullptr,                             // pTexelBufferView
      }},
      {});
}

Camera::~Camera() {
  uniformBuffer.unmapMemory();
  uniformBuffer.destroy();

  auto descriptorPool = Context::getDescriptorManager().getPool(DESC_CAMERA);

  assert(descriptorPool != nullptr);

  Context::getDevice().freeDescriptorSets(*descriptorPool, this->descriptorSet);
}

void Camera::setPos(glm::vec3 pos) {
  cameraUniform.view = glm::translate(cameraUniform.view, pos - this->pos);
  this->pos = pos;
}

glm::vec3 Camera::getPos() const { return this->pos; }

void Camera::setFov(float fov) { this->fov = fov; }

float Camera::getFov() const { return this->fov; }

void Camera::lookAt(glm::vec3 point) {
  cameraUniform.view = glm::lookAt(this->pos, point, {0.0, -1.0, 0.0});

  memcpy(this->mapping, &this->cameraUniform, sizeof(CameraUniform));
}

void Camera::update(uint32_t width, uint32_t height) {
  cameraUniform.proj = glm::perspective(
      glm::radians(this->fov),
      (float)width / (float)height,
      this->near,
      this->far);

  memcpy(this->mapping, &this->cameraUniform, sizeof(CameraUniform));
}

void Camera::bind(CommandBuffer &commandBuffer, GraphicsPipeline &pipeline) {
  commandBuffer.bindDescriptorSets(
      vkr::PipelineBindPoint::eGraphics,
      pipeline.getLayout(),
      0,                   // firstIndex
      this->descriptorSet, // pDescriptorSets
      {});
}
