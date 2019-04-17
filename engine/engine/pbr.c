#include "pbr.h"
#include "pipelines.h"
#include <fstd_util.h>
#include <renderer/context.h>
#include <renderer/pipeline.h>
#include <renderer/util.h>
#include <renderer/window.h>
#include <string.h>

void eg_pbr_model_init(eg_pbr_model_t *model, mat4_t transform) {
  model->uniform.transform = transform;

  {
    VkDescriptorSetLayout set_layouts[ARRAY_SIZE(model->descriptor_sets)];
    for (size_t i = 0; i < ARRAY_SIZE(model->descriptor_sets); i++) {
      set_layouts[i] = g_default_pipeline_layouts.pbr.set_layouts[2];
    }

    VkDescriptorSetAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.descriptorPool = g_ctx.descriptor_pool;
    alloc_info.descriptorSetCount = ARRAY_SIZE(model->descriptor_sets);
    alloc_info.pSetLayouts = set_layouts;

    VK_CHECK(vkAllocateDescriptorSets(
        g_ctx.device, &alloc_info, model->descriptor_sets));
  }

  for (uint32_t i = 0; i < RE_MAX_FRAMES_IN_FLIGHT; i++) {
    re_buffer_init(
        &model->buffers[i],
        &(re_buffer_options_t){
            .type = RE_BUFFER_TYPE_UNIFORM,
            .size = sizeof(model->uniform),
        });

    VkDescriptorBufferInfo buffer_info = {
        model->buffers[i].buffer, 0, sizeof(model->uniform)};

    VkWriteDescriptorSet descriptor_write = {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        NULL,
        model->descriptor_sets[i],         // dstSet
        0,                                 // dstBinding
        0,                                 // dstArrayElement
        1,                                 // descriptorCount
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
        NULL,                              // pImageInfo
        &buffer_info,                      // pBufferInfo
        NULL,                              // pTexelBufferView
    };

    vkUpdateDescriptorSets(g_ctx.device, 1, &descriptor_write, 0, NULL);

    re_buffer_map_memory(&model->buffers[i], &model->mappings[i]);
  }
}

void eg_pbr_model_update_uniform(
    eg_pbr_model_t *model, const eg_cmd_info_t *cmd_info) {
  memcpy(
      model->mappings[cmd_info->frame_index],
      &model->uniform,
      sizeof(model->uniform));
}

void eg_pbr_model_bind(
    eg_pbr_model_t *model,
    const eg_cmd_info_t *cmd_info,
    struct re_pipeline_t *pipeline,
    uint32_t set_index) {
  vkCmdBindDescriptorSets(
      cmd_info->cmd_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline->layout.layout,
      set_index, // firstSet
      1,
      &model->descriptor_sets[cmd_info->frame_index],
      0,
      NULL);
}

void eg_pbr_model_destroy(eg_pbr_model_t *model) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  vkFreeDescriptorSets(
      g_ctx.device,
      g_ctx.descriptor_pool,
      ARRAY_SIZE(model->descriptor_sets),
      model->descriptor_sets);

  for (uint32_t i = 0; i < RE_MAX_FRAMES_IN_FLIGHT; i++) {
    re_buffer_unmap_memory(&model->buffers[i]);
    re_buffer_destroy(&model->buffers[i]);
  }
}
