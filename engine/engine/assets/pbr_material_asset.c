#include "pbr_material_asset.h"
#include "../engine.h"
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

  if (albedo_texture == NULL) {
    albedo_texture = &g_eng.white_texture;
  }

  if (normal_texture == NULL) {
    material->uniform.has_normal_texture = 0.0f;
    normal_texture = &g_eng.white_texture;
  }

  if (metallic_roughness_texture == NULL) {
    metallic_roughness_texture = &g_eng.white_texture;
  }

  if (occlusion_texture == NULL) {
    occlusion_texture = &g_eng.white_texture;
  }

  if (emissive_texture == NULL) {
    emissive_texture = &g_eng.black_texture;
  }

  VkDescriptorSetLayout set_layout =
      g_default_pipeline_layouts.pbr.set_layouts[4];
  VkDescriptorUpdateTemplate update_template =
      g_default_pipeline_layouts.pbr.update_templates[4];

  {
    VkDescriptorSetLayout set_layouts[ARRAY_SIZE(material->descriptor_sets)];
    for (size_t i = 0; i < ARRAY_SIZE(material->descriptor_sets); i++) {
      set_layouts[i] = set_layout;
    }

    VK_CHECK(vkAllocateDescriptorSets(
        g_ctx.device,
        &(VkDescriptorSetAllocateInfo){
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = g_ctx.descriptor_pool,
            .descriptorSetCount = ARRAY_SIZE(material->descriptor_sets),
            .pSetLayouts = set_layouts,
        },
        material->descriptor_sets));
  }

  for (uint32_t i = 0; i < RE_MAX_FRAMES_IN_FLIGHT; i++) {
    re_buffer_init(
        &material->uniform_buffers[i],
        &(re_buffer_options_t){
            .type = RE_BUFFER_TYPE_UNIFORM,
            .size = sizeof(material->uniform),
        });

    re_buffer_map_memory(&material->uniform_buffers[i], &material->mappings[i]);

    vkUpdateDescriptorSetWithTemplate(
        g_ctx.device,
        material->descriptor_sets[i],
        update_template,
        (re_descriptor_info_t[]){
            {.image = albedo_texture->descriptor},
            {.image = normal_texture->descriptor},
            {.image = metallic_roughness_texture->descriptor},
            {.image = occlusion_texture->descriptor},
            {.image = emissive_texture->descriptor},
            {.buffer = {.buffer = material->uniform_buffers[i].buffer,
                        .offset = 0,
                        .range = sizeof(material->uniform)}},
        });
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
