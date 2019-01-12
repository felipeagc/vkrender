#include "camera_component.hpp"
#include "../scene.hpp"
#include <renderer/context.hpp>
#include <renderer/pipeline.hpp>
#include <renderer/util.hpp>
#include <renderer/window.hpp>

using namespace engine;

template <>
void engine::loadComponent<CameraComponent>(
    const sdf::Component &,
    ecs::World &world,
    AssetManager &,
    ecs::Entity entity) {
  world.assign<engine::CameraComponent>(entity);
}

CameraComponent::CameraComponent(float fov) : m_fov(fov) {
  auto &set_layout = renderer::ctx().resource_manager.set_layouts.camera;

  for (uint32_t i = 0; i < renderer::MAX_FRAMES_IN_FLIGHT; i++) {
    re_buffer_init_uniform(&m_uniformBuffers[i], sizeof(CameraUniform));
    re_buffer_map_memory(&m_uniformBuffers[i], &m_mappings[i]);

    this->m_descriptorSets[i] = re_allocate_resource_set(&set_layout);

    VkDescriptorBufferInfo bufferInfo{
        m_uniformBuffers[i].buffer,
        0,
        sizeof(CameraUniform),
    };

    VkWriteDescriptorSet descriptorWrite = {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        nullptr,
        m_descriptorSets[i].descriptor_set, // dstSet
        0,                                  // dstBinding
        0,                                  // dstArrayElement
        1,                                  // descriptorCount
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // descriptorType
        nullptr,                            // pImageInfo
        &bufferInfo,                        // pBufferInfo
        nullptr,                            // pTexelBufferView
    };

    vkUpdateDescriptorSets(
        renderer::ctx().m_device, 1, &descriptorWrite, 0, nullptr);
  }
}

CameraComponent::~CameraComponent() {
  VK_CHECK(vkDeviceWaitIdle(renderer::ctx().m_device));

  for (size_t i = 0; i < ARRAYSIZE(m_uniformBuffers); i++) {
    re_buffer_unmap_memory(&m_uniformBuffers[i]);
    re_buffer_destroy(&m_uniformBuffers[i]);
  }

  auto &set_layout = renderer::ctx().resource_manager.set_layouts.camera;

  for (auto &set : this->m_descriptorSets) {
    re_free_resource_set(&set_layout, &set);
  }
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

  // @note: See:
  // https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/
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

void CameraComponent::bind(renderer::Window &window, re_pipeline_t &pipeline) {
  auto i = window.getCurrentFrameIndex();
  VkCommandBuffer commandBuffer = window.getCurrentCommandBuffer();

  vkCmdBindDescriptorSets(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline.layout,
      0,
      1,
      &m_descriptorSets[i].descriptor_set,
      0,
      nullptr);
}
