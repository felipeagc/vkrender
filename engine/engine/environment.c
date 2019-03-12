#include "environment.h"
#include "assets/environment_asset.h"
#include "engine.h"
#include <fstd_util.h>
#include <renderer/context.h>
#include <renderer/pipeline.h>
#include <renderer/util.h>
#include <renderer/window.h>

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

  {
    VkDescriptorSetLayout set_layouts[ARRAYSIZE(environment->descriptor_sets)];
    for (size_t i = 0; i < ARRAYSIZE(environment->descriptor_sets); i++) {
      set_layouts[i] = g_eng.set_layouts.environment;
    }

    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.descriptorPool = g_ctx.descriptor_pool;
    alloc_info.descriptorSetCount = ARRAYSIZE(environment->descriptor_sets);
    alloc_info.pSetLayouts = set_layouts;

    VK_CHECK(vkAllocateDescriptorSets(
        g_ctx.device, &alloc_info, environment->descriptor_sets));
  }

  // Update descriptor sets
  for (size_t i = 0; i < ARRAYSIZE(environment->descriptor_sets); i++) {
    re_buffer_init_uniform(
        &environment->uniform_buffers[i], sizeof(eg_environment_uniform_t));
    re_buffer_map_memory(
        &environment->uniform_buffers[i], &environment->mappings[i]);

    VkDescriptorBufferInfo buffer_info = {
        environment->uniform_buffers[i].buffer,
        0,
        sizeof(eg_environment_uniform_t),
    };

    VkDescriptorImageInfo skybox_descriptor_info =
        asset->skybox_cubemap.descriptor;
    VkDescriptorImageInfo irradiance_descriptor_info =
        asset->irradiance_cubemap.descriptor;
    VkDescriptorImageInfo radiance_descriptor_info =
        asset->radiance_cubemap.descriptor;
    VkDescriptorImageInfo brdf_lut_descriptor_info = asset->brdf_lut.descriptor;

    VkWriteDescriptorSet descriptor_writes[] = {
        (VkWriteDescriptorSet){
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            NULL,
            environment->descriptor_sets[i],   // dstSet
            0,                                 // dstBinding
            0,                                 // dstArrayElement
            1,                                 // descriptorCount
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
            NULL,                              // pImageInfo
            &buffer_info,                      // pBufferInfo
            NULL,                              // pTexelBufferView
        },
        (VkWriteDescriptorSet){
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            NULL,
            environment->descriptor_sets[i],           // dstSet
            1,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &skybox_descriptor_info,                   // pImageInfo
            NULL,                                      // pBufferInfo
            NULL,                                      // pTexelBufferView
        },
        (VkWriteDescriptorSet){
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            NULL,
            environment->descriptor_sets[i],           // dstSet
            2,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &irradiance_descriptor_info,               // pImageInfo
            NULL,                                      // pBufferInfo
            NULL,                                      // pTexelBufferView
        },
        (VkWriteDescriptorSet){
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            NULL,
            environment->descriptor_sets[i],           // dstSet
            3,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &radiance_descriptor_info,                 // pImageInfo
            NULL,                                      // pBufferInfo
            NULL,                                      // pTexelBufferView
        },
        (VkWriteDescriptorSet){
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            NULL,
            environment->descriptor_sets[i],           // dstSet
            4,                                         // dstBinding
            0,                                         // dstArrayElement
            1,                                         // descriptorCount
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
            &brdf_lut_descriptor_info,                 // pImageInfo
            NULL,                                      // pBufferInfo
            NULL,                                      // pTexelBufferView
        },
    };

    vkUpdateDescriptorSets(
        g_ctx.device, ARRAYSIZE(descriptor_writes), descriptor_writes, 0, NULL);
  }
}

void eg_environment_update(
    eg_environment_t *environment, struct re_window_t *window) {
  memcpy(
      environment->mappings[window->current_frame],
      &environment->uniform,
      sizeof(eg_environment_uniform_t));
}

void eg_environment_bind(
    eg_environment_t *environment,
    struct re_window_t *window,
    struct re_pipeline_t *pipeline,
    uint32_t set_index) {
  VkCommandBuffer command_buffer = re_window_get_current_command_buffer(window);

  uint32_t i = window->current_frame;

  vkCmdBindPipeline(
      command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);

  vkCmdBindDescriptorSets(
      command_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline->layout,
      set_index, // firstSet
      1,
      &environment->descriptor_sets[i],
      0,
      NULL);
}

void eg_environment_draw_skybox(
    eg_environment_t *environment,
    struct re_window_t *window,
    struct re_pipeline_t *pipeline) {
  VkCommandBuffer command_buffer = re_window_get_current_command_buffer(window);

  uint32_t i = window->current_frame;

  vkCmdBindPipeline(
      command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);

  vkCmdBindDescriptorSets(
      command_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline->layout,
      1, // firstSet
      1,
      &environment->descriptor_sets[i],
      0,
      NULL);

  vkCmdDraw(command_buffer, 36, 1, 0, 0);
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

  for (size_t i = 0; i < ARRAYSIZE(environment->uniform_buffers); i++) {
    re_buffer_unmap_memory(&environment->uniform_buffers[i]);
    re_buffer_destroy(&environment->uniform_buffers[i]);
  }

  vkFreeDescriptorSets(
      g_ctx.device,
      g_ctx.descriptor_pool,
      ARRAYSIZE(environment->descriptor_sets),
      environment->descriptor_sets);
}
