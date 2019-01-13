#include "environment_component.hpp"

#include "../assets/environment_asset.hpp"
#include "../assets/texture_asset.hpp"
#include "../scene.hpp"
#include <ftl/logging.hpp>
#include <renderer/context.hpp>
#include <renderer/util.hpp>
#include <renderer/window.hpp>

using namespace engine;

template <>
void engine::loadComponent<EnvironmentComponent>(
    const sdf::Component &comp,
    ecs::World &world,
    AssetManager &assetManager,
    ecs::Entity entity) {
  for (auto &prop : comp.properties) {
    if (strcmp(prop.name, "asset") == 0) {
      uint32_t assetId;
      prop.get_uint32(&assetId);

      world.assign<EnvironmentComponent>(
          entity, assetManager.getAsset<EnvironmentAsset>(assetId));
    }
  }
}

EnvironmentComponent::EnvironmentComponent(
    const EnvironmentAsset &environmentAsset)
    : m_environmentAssetIndex(environmentAsset.index) {
  // Allocate descriptor sets
  auto &set_layout = g_ctx.resource_manager.set_layouts.environment;

  m_ubo.radianceMipLevels =
      (float)environmentAsset.m_radianceCubemap.mip_levels;

  for (size_t i = 0; i < ARRAYSIZE(m_descriptorSets); i++) {
    this->m_descriptorSets[i] = re_allocate_resource_set(&set_layout);
  }

  // Update descriptor sets
  for (size_t i = 0; i < ARRAYSIZE(m_descriptorSets); i++) {
    re_buffer_init_uniform(&m_uniformBuffers[i], sizeof(EnvironmentUniform));
    re_buffer_map_memory(&m_uniformBuffers[i], &m_mappings[i]);

    VkDescriptorBufferInfo bufferInfo{
        m_uniformBuffers[i].buffer,
        0,
        sizeof(EnvironmentUniform),
    };

    auto skyboxDescriptorInfo =
        re_cubemap_descriptor(&environmentAsset.m_skyboxCubemap);
    auto irradianceDescriptorInfo =
        re_cubemap_descriptor(&environmentAsset.m_irradianceCubemap);
    auto radianceDescriptorInfo =
        re_cubemap_descriptor(&environmentAsset.m_radianceCubemap);
    auto brdfLutDescriptorInfo =
        re_texture_descriptor(&environmentAsset.m_brdfLut);

    VkWriteDescriptorSet descriptorWrites[] = {
        VkWriteDescriptorSet{
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
        },
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            m_descriptorSets[i].descriptor_set,        // dstSet
            1,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &skyboxDescriptorInfo,                     // pImageInfo
            nullptr,                                   // pBufferInfo
            nullptr,                                   // pTexelBufferView
        },
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            m_descriptorSets[i].descriptor_set,        // dstSet
            2,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &irradianceDescriptorInfo,                 // pImageInfo
            nullptr,                                   // pBufferInfo
            nullptr,                                   // pTexelBufferView
        },
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            m_descriptorSets[i].descriptor_set,        // dstSet
            3,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &radianceDescriptorInfo,                   // pImageInfo
            nullptr,                                   // pBufferInfo
            nullptr,                                   // pTexelBufferView
        },
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            m_descriptorSets[i].descriptor_set,        // dstSet
            4,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &brdfLutDescriptorInfo,                    // pImageInfo
            nullptr,                                   // pBufferInfo
            nullptr,                                   // pTexelBufferView
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

EnvironmentComponent::~EnvironmentComponent() {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  for (size_t i = 0; i < ARRAYSIZE(m_uniformBuffers); i++) {
    re_buffer_unmap_memory(&m_uniformBuffers[i]);
    re_buffer_destroy(&m_uniformBuffers[i]);
  }

  auto &set_layout = g_ctx.resource_manager.set_layouts.environment;

  for (uint32_t i = 0; i < renderer::MAX_FRAMES_IN_FLIGHT; i++) {
    re_free_resource_set(&set_layout, &m_descriptorSets[i]);
  }
}

void EnvironmentComponent::bind(
    const re_window_t *window, re_pipeline_t &pipeline, uint32_t setIndex) {
  auto commandBuffer = re_window_get_current_command_buffer(window);

  auto i = window->current_frame;

  vkCmdBindPipeline(
      commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

  vkCmdBindDescriptorSets(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline.layout,
      setIndex, // firstSet
      1,
      &m_descriptorSets[i].descriptor_set,
      0,
      nullptr);
}

void EnvironmentComponent::drawSkybox(
    const re_window_t *window, re_pipeline_t &pipeline) {
  auto commandBuffer = re_window_get_current_command_buffer(window);

  auto i = window->current_frame;

  this->update(window);

  vkCmdBindPipeline(
      commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

  vkCmdBindDescriptorSets(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline.layout,
      1, // firstSet
      1,
      &m_descriptorSets[i].descriptor_set,
      0,
      nullptr);

  vkCmdDraw(commandBuffer, 36, 1, 0, 0);
}

void EnvironmentComponent::update(const re_window_t *window) {
  auto i = window->current_frame;
  memcpy(m_mappings[i], &m_ubo, sizeof(EnvironmentUniform));
}

void EnvironmentComponent::addLight(
    const glm::vec3 &pos, const glm::vec3 &color) {
  m_ubo.lights[m_ubo.lightCount] =
      Light{glm::vec4(pos, 1.0), glm::vec4(color, 1.0)};
  m_ubo.lightCount++;
}

void EnvironmentComponent::resetLights() { m_ubo.lightCount = 0; }
