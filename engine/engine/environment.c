#include "environment.h"
#include "assets/environment_asset.h"
#include "pipelines.h"
#include <fstd_util.h>
#include <renderer/context.h>
#include <renderer/pipeline.h>
#include <renderer/util.h>
#include <renderer/window.h>
#include <string.h>

void eg_environment_init(
    eg_environment_t *environment, eg_environment_asset_t *asset) {
  environment->uniform.sun_direction = (vec3_t){0.0, -1.0, 0.0};
  environment->uniform.exposure = 8.0f;
  environment->uniform.sun_color = (vec3_t){1.0f, 1.0f, 1.0f};
  environment->uniform.sun_intensity = 1.0f;
  environment->uniform.radiance_mip_levels = 1.0f;
  environment->uniform.point_light_count = 0;

  environment->uniform.radiance_mip_levels =
      (float)asset->radiance_cubemap.mip_level_count;

  VkDescriptorSetLayout set_layout =
      g_default_pipeline_layouts.skybox.set_layouts[1];
  VkDescriptorUpdateTemplate update_template =
      g_default_pipeline_layouts.skybox.update_templates[1];

  {
    VkDescriptorSetLayout set_layouts[ARRAY_SIZE(environment->descriptor_sets)];
    for (size_t i = 0; i < ARRAY_SIZE(environment->descriptor_sets); i++) {
      set_layouts[i] = set_layout;
    }

    VK_CHECK(vkAllocateDescriptorSets(
        g_ctx.device,
        &(VkDescriptorSetAllocateInfo){
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = g_ctx.descriptor_pool,
            .descriptorSetCount = ARRAY_SIZE(environment->descriptor_sets),
            .pSetLayouts = set_layouts,
        },
        environment->descriptor_sets));
  }

  // Update descriptor sets
  for (size_t i = 0; i < ARRAY_SIZE(environment->descriptor_sets); i++) {
    re_buffer_init(
        &environment->uniform_buffers[i],
        &(re_buffer_options_t){
            .type = RE_BUFFER_TYPE_UNIFORM,
            .size = sizeof(eg_environment_uniform_t),
        });
    re_buffer_map_memory(
        &environment->uniform_buffers[i], &environment->mappings[i]);

    vkUpdateDescriptorSetWithTemplate(
        g_ctx.device,
        environment->descriptor_sets[i],
        update_template,
        (re_descriptor_update_info_t[]){
            {.buffer_info = {.buffer = environment->uniform_buffers[i].buffer,
                             .offset = 0,
                             .range = sizeof(eg_environment_uniform_t)}},
            {.image_info = asset->skybox_cubemap.descriptor},
            {.image_info = asset->irradiance_cubemap.descriptor},
            {.image_info = asset->radiance_cubemap.descriptor},
            {.image_info = asset->brdf_lut.descriptor},
        });
  }
}

void eg_environment_update(
    eg_environment_t *environment, const eg_cmd_info_t *cmd_info) {
  memcpy(
      environment->mappings[cmd_info->frame_index],
      &environment->uniform,
      sizeof(eg_environment_uniform_t));
}

void eg_environment_bind(
    eg_environment_t *environment,
    const eg_cmd_info_t *cmd_info,
    struct re_pipeline_t *pipeline,
    uint32_t set_index) {
  vkCmdBindPipeline(
      cmd_info->cmd_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline->pipeline);

  vkCmdBindDescriptorSets(
      cmd_info->cmd_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline->layout.layout,
      set_index, // firstSet
      1,
      &environment->descriptor_sets[cmd_info->frame_index],
      0,
      NULL);
}

void eg_environment_draw_skybox(
    eg_environment_t *environment,
    const eg_cmd_info_t *cmd_info,
    struct re_pipeline_t *pipeline) {
  vkCmdBindPipeline(
      cmd_info->cmd_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline->pipeline);

  vkCmdBindDescriptorSets(
      cmd_info->cmd_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline->layout.layout,
      1, // firstSet
      1,
      &environment->descriptor_sets[cmd_info->frame_index],
      0,
      NULL);

  vkCmdDraw(cmd_info->cmd_buffer, 36, 1, 0, 0);
}

bool eg_environment_add_point_light(
    eg_environment_t *environment, const vec3_t pos, const vec3_t color) {
  if (environment->uniform.point_light_count + 1 == EG_MAX_POINT_LIGHTS) {
    return false;
  }

  environment->uniform.point_lights[environment->uniform.point_light_count] =
      (eg_point_light_t){
          (vec4_t){pos.x, pos.y, pos.z, 1.0},
          (vec4_t){color.x, color.y, color.z, 1.0},
      };
  environment->uniform.point_light_count++;
  return true;
}

void eg_environment_reset_point_lights(eg_environment_t *environment) {
  environment->uniform.point_light_count = 0;
}

void eg_environment_destroy(eg_environment_t *environment) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  for (size_t i = 0; i < ARRAY_SIZE(environment->uniform_buffers); i++) {
    re_buffer_unmap_memory(&environment->uniform_buffers[i]);
    re_buffer_destroy(&environment->uniform_buffers[i]);
  }

  vkFreeDescriptorSets(
      g_ctx.device,
      g_ctx.descriptor_pool,
      ARRAY_SIZE(environment->descriptor_sets),
      environment->descriptor_sets);
}
