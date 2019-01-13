#include "billboard_component.hpp"
#include "../scene.hpp"
#include <ftl/logging.hpp>
#include <renderer/context.hpp>
#include <renderer/util.hpp>

using namespace engine;

template <>
void engine::loadComponent<BillboardComponent>(
    const sdf::Component &comp,
    ecs::World &world,
    AssetManager &assetManager,
    ecs::Entity entity) {
  for (auto &prop : comp.properties) {
    if (strcmp(prop.name, "asset") == 0) {
      uint32_t assetId;
      prop.get_uint32(&assetId);
      world.assign<engine::BillboardComponent>(
          entity, assetManager.getAsset<engine::TextureAsset>(assetId));
    }
  }
}

BillboardComponent::BillboardComponent(const TextureAsset &textureAsset)
    : m_textureIndex(textureAsset.index) {
  // Allocate material descriptor sets
  auto &set_layout = g_ctx.resource_manager.set_layouts.material;
  for (size_t i = 0; i < ARRAYSIZE(m_materialDescriptorSets); i++) {
    m_materialDescriptorSets[i] = re_allocate_resource_set(&set_layout);
  }

  // Update descriptor sets
  for (size_t i = 0; i < ARRAYSIZE(m_materialDescriptorSets); i++) {
    auto albedoDescriptorInfo = re_texture_descriptor(&textureAsset.m_texture);

    VkWriteDescriptorSet descriptorWrites[] = {
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            m_materialDescriptorSets[i].descriptor_set, // dstSet
            0,                                          // dstBinding
            0,                                          // dstArrayElement
            1,                                          // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  // descriptorType
            &albedoDescriptorInfo,                      // pImageInfo
            nullptr,                                    // pBufferInfo
            nullptr,                                    // pTexelBufferView
        },
    };

    vkUpdateDescriptorSets(
        g_ctx.device,
        ARRAYSIZE(descriptorWrites),
        descriptorWrites,
        0,
        nullptr);
  }
}

BillboardComponent::~BillboardComponent() {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  auto &set_layout = g_ctx.resource_manager.set_layouts.material;
  for (auto &set : m_materialDescriptorSets) {
    re_free_resource_set(&set_layout, &set);
  }
}

void BillboardComponent::draw(
    const re_window_t *window,
    re_pipeline_t pipeline,
    const glm::mat4 &transform,
    const glm::vec3 &color) {
  m_ubo.model = transform;
  m_ubo.color = glm::vec4(color, 1.0f);

  auto commandBuffer = re_window_get_current_command_buffer(window);

  auto i = window->current_frame;

  vkCmdBindPipeline(
      commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

  vkCmdPushConstants(
      commandBuffer,
      pipeline.layout,
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      0,
      sizeof(BillboardUniform),
      &m_ubo);

  vkCmdBindDescriptorSets(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline.layout,
      1, // firstSet
      1,
      &m_materialDescriptorSets[i].descriptor_set,
      0,
      nullptr);

  vkCmdDraw(commandBuffer, 6, 1, 0, 0);
}
