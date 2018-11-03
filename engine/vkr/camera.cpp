#include "camera.hpp"
#include "commandbuffer.hpp"
#include "context.hpp"
#include "window.hpp"

using namespace vkr;

Camera::Camera(glm::vec3 position) : pos(position) {
  auto [descriptorPool, descriptorSetLayout] =
      Context::getDescriptorManager()[DESC_CAMERA];

  assert(descriptorPool != nullptr && descriptorSetLayout != nullptr);

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    this->uniformBuffers[i] = Buffer{
        sizeof(CameraUniform),
        vkr::BufferUsageFlagBits::eUniformBuffer,
        vkr::MemoryUsageFlagBits::eCpuToGpu,
        vkr::MemoryPropertyFlagBits::eHostCoherent,
    };

    this->uniformBuffers[i].mapMemory(&this->mappings[i]);

    DescriptorBufferInfo bufferInfo{
        this->uniformBuffers[i].getHandle(),
        0,
        sizeof(CameraUniform),
    };

    this->descriptorSets[i] =
        descriptorPool->allocateDescriptorSets(1, *descriptorSetLayout)[0];

    Context::getDevice().updateDescriptorSets(
        {vk::WriteDescriptorSet{
            this->descriptorSets[i],             // dstSet
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
  for (auto &uniformBuffer : uniformBuffers) {
    uniformBuffer.unmapMemory();
    uniformBuffer.destroy();
  }

  auto descriptorPool = Context::getDescriptorManager().getPool(DESC_CAMERA);

  assert(descriptorPool != nullptr);

  Context::getDevice().freeDescriptorSets(
      *descriptorPool, this->descriptorSets);
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
}

void Camera::update(Window &window) {
  auto i = window.getCurrentFrameIndex();

  cameraUniform.proj = glm::perspective(
      glm::radians(this->fov),
      static_cast<float>(window.getWidth()) /
          static_cast<float>(window.getHeight()),
      this->near,
      this->far);

  memcpy(this->mappings[i], &this->cameraUniform, sizeof(CameraUniform));
}

void Camera::bind(Window &window, GraphicsPipeline &pipeline) {
  auto i = window.getCurrentFrameIndex();

  window.getCurrentCommandBuffer().bindDescriptorSets(
      vkr::PipelineBindPoint::eGraphics,
      pipeline.getLayout(),
      0,                       // firstIndex
      this->descriptorSets[i], // pDescriptorSets
      {});
}
