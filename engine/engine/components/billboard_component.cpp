#include "billboard_component.hpp"
#include "../scene.hpp"
#include <fstl/logging.hpp>
#include <renderer/context.hpp>
#include <renderer/util.hpp>
#include <renderer/window.hpp>

using namespace engine;

template <>
void engine::loadComponent<BillboardComponent>(
    const scene::Component &comp,
    ecs::World &world,
    AssetManager &assetManager,
    ecs::Entity entity) {
  world.assign<engine::BillboardComponent>(
      entity,
      assetManager.getAsset<engine::TextureAsset>(
          comp.properties.at("asset").getUint32()));
}

BillboardComponent::BillboardComponent(const TextureAsset &textureAsset)
    : m_textureIndex(textureAsset.index()) {
  // Allocate material descriptor sets
  auto &setLayout = renderer::ctx().m_resourceManager.m_setLayouts.material;
  for (size_t i = 0; i < ARRAYSIZE(m_materialDescriptorSets); i++) {
    m_materialDescriptorSets[i] = setLayout.allocate();
  }

  // Update descriptor sets
  for (size_t i = 0; i < ARRAYSIZE(m_materialDescriptorSets); i++) {
    auto albedoDescriptorInfo = textureAsset.texture().getDescriptorInfo();

    VkWriteDescriptorSet descriptorWrites[] = {
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            m_materialDescriptorSets[i],               // dstSet
            0,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &albedoDescriptorInfo,                     // pImageInfo
            nullptr,                                   // pBufferInfo
            nullptr,                                   // pTexelBufferView
        },
    };

    vkUpdateDescriptorSets(
        renderer::ctx().m_device,
        ARRAYSIZE(descriptorWrites),
        descriptorWrites,
        0,
        nullptr);
  }
}

BillboardComponent::~BillboardComponent() {
  VK_CHECK(vkDeviceWaitIdle(renderer::ctx().m_device));

  auto &setLayout = renderer::ctx().m_resourceManager.m_setLayouts.material;
  for (auto &set : m_materialDescriptorSets) {
    setLayout.free(set);
  }
}

void BillboardComponent::draw(
    renderer::Window &window,
    renderer::GraphicsPipeline &pipeline,
    const glm::mat4 &transform,
    const glm::vec3 &color) {
  m_ubo.model = transform;
  m_ubo.color = glm::vec4(color, 1.0f);

  auto commandBuffer = window.getCurrentCommandBuffer();

  auto i = window.getCurrentFrameIndex();

  vkCmdBindPipeline(
      commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.m_pipeline);

  vkCmdPushConstants(
      commandBuffer,
      pipeline.m_pipelineLayout,
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      0,
      sizeof(BillboardUniform),
      &m_ubo);

  vkCmdBindDescriptorSets(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline.m_pipelineLayout,
      1, // firstSet
      1,
      m_materialDescriptorSets[i],
      0,
      nullptr);

  vkCmdDraw(commandBuffer, 6, 1, 0, 0);
}
