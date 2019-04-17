#include "pbr_material_asset.h"
#include "../pipelines.h"
#include <fstd_util.h>
#include <renderer/context.h>
#include <renderer/image.h>
#include <renderer/pipeline.h>
#include <renderer/util.h>
#include <renderer/window.h>
#include <string.h>

void eg_pbr_material_asset_init(
    eg_pbr_material_asset_t *material,
    re_image_t *albedo_texture,
    re_image_t *normal_texture,
    re_image_t *metallic_roughness_texture,
    re_image_t *occlusion_texture,
    re_image_t *emissive_texture) {
  material->uniform.base_color_factor = (vec4_t){1.0, 1.0, 1.0, 1.0};
  material->uniform.metallic = 1.0;
  material->uniform.roughness = 1.0;
  material->uniform.emissive_factor = (vec4_t){1.0, 1.0, 1.0, 1.0};
  material->uniform.has_normal_texture = 1.0f;

  for (uint32_t i = 0; i < ARRAY_SIZE(material->uniform_buffers); i++) {
    re_buffer_init(
        &material->uniform_buffers[i],
        &(re_buffer_options_t){
            .type = RE_BUFFER_TYPE_UNIFORM,
            .size = sizeof(material->uniform),
        });

    re_buffer_map_memory(&material->uniform_buffers[i], &material->mappings[i]);
  }

  if (albedo_texture == NULL) {
    albedo_texture = &g_ctx.white_texture;
  }

  if (normal_texture == NULL) {
    material->uniform.has_normal_texture = 0.0f;
    normal_texture = &g_ctx.white_texture;
  }

  if (metallic_roughness_texture == NULL) {
    metallic_roughness_texture = &g_ctx.white_texture;
  }

  if (occlusion_texture == NULL) {
    occlusion_texture = &g_ctx.white_texture;
  }

  if (emissive_texture == NULL) {
    emissive_texture = &g_ctx.black_texture;
  }

  {
    VkDescriptorSetLayout set_layouts[ARRAY_SIZE(material->descriptor_sets)];
    for (size_t i = 0; i < ARRAY_SIZE(material->descriptor_sets); i++) {
      set_layouts[i] = g_default_pipeline_layouts.pbr.set_layouts[4];
    }

    VkDescriptorSetAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.descriptorPool = g_ctx.descriptor_pool;
    alloc_info.descriptorSetCount = ARRAY_SIZE(material->descriptor_sets);
    alloc_info.pSetLayouts = set_layouts;

    VK_CHECK(vkAllocateDescriptorSets(
        g_ctx.device, &alloc_info, material->descriptor_sets));
  }

  for (uint32_t i = 0; i < RE_MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorImageInfo albedo_descriptor = albedo_texture->descriptor;
    VkDescriptorImageInfo normal_descriptor = normal_texture->descriptor;
    VkDescriptorImageInfo metallic_roughness_descriptor =
        metallic_roughness_texture->descriptor;
    VkDescriptorImageInfo occlusion_descriptor = occlusion_texture->descriptor;
    VkDescriptorImageInfo emissive_descriptor = emissive_texture->descriptor;
    VkDescriptorBufferInfo uniform_buffer_descriptor = {
        .buffer = material->uniform_buffers[i].buffer,
        .offset = 0,
        .range = sizeof(material->uniform),
    };

    VkWriteDescriptorSet descriptor_writes[] = {
        (VkWriteDescriptorSet){
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            NULL,
            material->descriptor_sets[i],              // dstSet
            0,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &albedo_descriptor,                        // pImageInfo
            NULL,                                      // pBufferInfo
            NULL,                                      // pTexelBufferView
        },
        (VkWriteDescriptorSet){
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            NULL,
            material->descriptor_sets[i],              // dstSet
            1,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &normal_descriptor,                        // pImageInfo
            NULL,                                      // pBufferInfo
            NULL,                                      // pTexelBufferView
        },
        (VkWriteDescriptorSet){
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            NULL,
            material->descriptor_sets[i],              // dstSet
            2,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &metallic_roughness_descriptor,            // pImageInfo
            NULL,                                      // pBufferInfo
            NULL,                                      // pTexelBufferView
        },
        (VkWriteDescriptorSet){
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            NULL,
            material->descriptor_sets[i],              // dstSet
            3,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &occlusion_descriptor,                     // pImageInfo
            NULL,                                      // pBufferInfo
            NULL,                                      // pTexelBufferView
        },
        (VkWriteDescriptorSet){
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            NULL,
            material->descriptor_sets[i],              // dstSet
            4,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &emissive_descriptor,                      // pImageInfo
            NULL,                                      // pBufferInfo
            NULL,                                      // pTexelBufferView
        },
        (VkWriteDescriptorSet){
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            NULL,
            material->descriptor_sets[i],      // dstSet
            5,                                 // dstBinding
            0,                                 // dstArrayElement
            1,                                 // descriptorCount
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
            NULL,                              // pImageInfo
            &uniform_buffer_descriptor,        // pBufferInfo
            NULL,                              // pTexelBufferView
        },
    };

    vkUpdateDescriptorSets(
        g_ctx.device,
        ARRAY_SIZE(descriptor_writes),
        descriptor_writes,
        0,
        NULL);
  }
}

void eg_pbr_material_asset_bind(
    eg_pbr_material_asset_t *material,
    const eg_cmd_info_t *cmd_info,
    re_pipeline_t *pipeline,
    uint32_t set_index) {
  memcpy(
      material->mappings[cmd_info->frame_index],
      &material->uniform,
      sizeof(material->uniform));

  vkCmdBindDescriptorSets(
      cmd_info->cmd_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline->layout.layout,
      set_index, // firstSet
      1,
      &material->descriptor_sets[cmd_info->frame_index],
      0,
      NULL);
}

void eg_pbr_material_asset_destroy(eg_pbr_material_asset_t *material) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  for (uint32_t i = 0; i < ARRAY_SIZE(material->uniform_buffers); i++) {
    re_buffer_unmap_memory(&material->uniform_buffers[i]);
    re_buffer_destroy(&material->uniform_buffers[i]);
  }

  vkFreeDescriptorSets(
      g_ctx.device,
      g_ctx.descriptor_pool,
      ARRAY_SIZE(material->descriptor_sets),
      material->descriptor_sets);
}
