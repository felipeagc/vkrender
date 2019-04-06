#include "pbr_material_asset.h"
#include "../engine.h"
#include <fstd_util.h>
#include <renderer/context.h>
#include <renderer/pipeline.h>
#include <renderer/image.h>
#include <renderer/util.h>
#include <renderer/window.h>

void eg_pbr_material_asset_init(
    eg_pbr_material_asset_t *material,
    re_image_t *albedo_texture,
    re_image_t *normal_texture,
    re_image_t *metallic_roughness_texture,
    re_image_t *occlusion_texture,
    re_image_t *emissive_texture) {
  eg_asset_init(&material->asset, EG_PBR_MATERIAL_ASSET_TYPE);

  material->uniform.base_color_factor = (vec4_t){1.0, 1.0, 1.0, 1.0};
  material->uniform.metallic = 1.0;
  material->uniform.roughness = 1.0;
  material->uniform.emissive_factor = (vec4_t){0.0, 0.0, 0.0, 0.0};
  material->uniform.has_normal_texture = 1.0f;

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
      set_layouts[i] = g_eng.set_layouts.material;
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
    };

    vkUpdateDescriptorSets(
        g_ctx.device, ARRAY_SIZE(descriptor_writes), descriptor_writes, 0, NULL);
  }
}

void eg_pbr_material_asset_bind(
    eg_pbr_material_asset_t *material,
    re_window_t *window,
    re_pipeline_t *pipeline,
    uint32_t set_index) {
  uint32_t i = window->current_frame;
  VkCommandBuffer command_buffer = re_window_get_current_command_buffer(window);

  vkCmdPushConstants(
      command_buffer,
      pipeline->layout,
      VK_SHADER_STAGE_ALL_GRAPHICS,
      0,
      sizeof(material->uniform),
      &material->uniform);

  vkCmdBindDescriptorSets(
      command_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline->layout,
      set_index, // firstSet
      1,
      &material->descriptor_sets[i],
      0,
      NULL);
}

void eg_pbr_material_asset_destroy(eg_pbr_material_asset_t *material) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  vkFreeDescriptorSets(
      g_ctx.device,
      g_ctx.descriptor_pool,
      ARRAY_SIZE(material->descriptor_sets),
      material->descriptor_sets);

  eg_asset_destroy(&material->asset);
}
