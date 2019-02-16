#include "pbr.hpp"
#include <renderer/pipeline.hpp>
#include <renderer/texture.hpp>
#include <renderer/window.hpp>

void eg_pbr_model_init(eg_pbr_model_t *model, mat4_t transform) {
  model->uniform.transform = transform;

  for (uint32_t i = 0; i < RE_MAX_FRAMES_IN_FLIGHT; i++) {
    re_buffer_init_uniform(&model->buffers[i], sizeof(model->uniform));

    model->resource_sets[i] =
        re_allocate_resource_set(&g_ctx.resource_manager.set_layouts.model);

    VkDescriptorBufferInfo buffer_info = {
        model->buffers[i].buffer, 0, sizeof(model->uniform)};

    VkWriteDescriptorSet descriptor_write = {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        nullptr,
        model->resource_sets[i].descriptor_set, // dstSet
        0,                                      // dstBinding
        0,                                      // dstArrayElement
        1,                                      // descriptorCount
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,      // descriptorType
        nullptr,                                // pImageInfo
        &buffer_info,                           // pBufferInfo
        nullptr,                                // pTexelBufferView
    };

    vkUpdateDescriptorSets(g_ctx.device, 1, &descriptor_write, 0, NULL);

    re_buffer_map_memory(&model->buffers[i], &model->mappings[i]);
  }
}

void eg_pbr_model_update_uniform(
    eg_pbr_model_t *model, struct re_window_t *window) {
  memcpy(
      model->mappings[window->current_frame],
      &model->uniform,
      sizeof(model->uniform));
}

void eg_pbr_model_bind(
    eg_pbr_model_t *model,
    struct re_window_t *window,
    struct re_pipeline_t *pipeline,
    uint32_t set_index) {
  uint32_t i = window->current_frame;
  VkCommandBuffer command_buffer = re_window_get_current_command_buffer(window);

  vkCmdBindDescriptorSets(
      command_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline->layout,
      set_index, // firstSet
      1,
      &model->resource_sets[i].descriptor_set,
      0,
      NULL);
}

void eg_pbr_model_destroy(eg_pbr_model_t *model) {
  for (uint32_t i = 0; i < RE_MAX_FRAMES_IN_FLIGHT; i++) {
    re_free_resource_set(
        &g_ctx.resource_manager.set_layouts.model, &model->resource_sets[i]);

    re_buffer_unmap_memory(&model->buffers[i]);

    re_buffer_destroy(&model->buffers[i]);
  }
}

void eg_pbr_material_asset_init(
    eg_pbr_material_asset_t *material,
    re_texture_t *albedo_texture,
    re_texture_t *normal_texture,
    re_texture_t *metallic_roughness_texture,
    re_texture_t *occlusion_texture,
    re_texture_t *emissive_texture) {
  eg_asset_init(
      &material->asset, (eg_asset_destructor_t)eg_pbr_material_asset_destroy);

  material->uniform.base_color_factor = {1.0, 1.0, 1.0, 1.0};
  material->uniform.metallic = 1.0;
  material->uniform.roughness = 1.0;
  material->uniform.emissive_factor = {0.0, 0.0, 0.0, 0.0};
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

  for (uint32_t i = 0; i < RE_MAX_FRAMES_IN_FLIGHT; i++) {
    material->resource_sets[i] =
        re_allocate_resource_set(&g_ctx.resource_manager.set_layouts.material);

    VkDescriptorImageInfo albedo_descriptor =
        re_texture_descriptor(albedo_texture);
    VkDescriptorImageInfo normal_descriptor =
        re_texture_descriptor(normal_texture);
    VkDescriptorImageInfo metallic_roughness_descriptor =
        re_texture_descriptor(metallic_roughness_texture);
    VkDescriptorImageInfo occlusion_descriptor =
        re_texture_descriptor(occlusion_texture);
    VkDescriptorImageInfo emissive_descriptor =
        re_texture_descriptor(emissive_texture);

    VkWriteDescriptorSet descriptor_writes[] = {
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            NULL,
            material->resource_sets[i].descriptor_set, // dstSet
            0,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &albedo_descriptor,                        // pImageInfo
            NULL,                                      // pBufferInfo
            NULL,                                      // pTexelBufferView
        },
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            NULL,
            material->resource_sets[i].descriptor_set, // dstSet
            1,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &normal_descriptor,                        // pImageInfo
            NULL,                                      // pBufferInfo
            NULL,                                      // pTexelBufferView
        },
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            NULL,
            material->resource_sets[i].descriptor_set, // dstSet
            2,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &metallic_roughness_descriptor,            // pImageInfo
            NULL,                                      // pBufferInfo
            NULL,                                      // pTexelBufferView
        },
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            NULL,
            material->resource_sets[i].descriptor_set, // dstSet
            3,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &occlusion_descriptor,                     // pImageInfo
            NULL,                                      // pBufferInfo
            NULL,                                      // pTexelBufferView
        },
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            NULL,
            material->resource_sets[i].descriptor_set, // dstSet
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
        g_ctx.device, ARRAYSIZE(descriptor_writes), descriptor_writes, 0, NULL);
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
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      0,
      sizeof(material->uniform),
      &material->uniform);

  vkCmdBindDescriptorSets(
      command_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline->layout,
      set_index, // firstSet
      1,
      &material->resource_sets[i].descriptor_set,
      0,
      NULL);
}

void eg_pbr_material_asset_destroy(eg_pbr_material_asset_t *material) {
  for (uint32_t i = 0; i < RE_MAX_FRAMES_IN_FLIGHT; i++) {
    re_free_resource_set(
        &g_ctx.resource_manager.set_layouts.material,
        &material->resource_sets[i]);
  }

  eg_asset_destroy(&material->asset);
}
