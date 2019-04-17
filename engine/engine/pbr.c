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

  VkDescriptorSetLayout set_layout =
      g_default_pipeline_layouts.pbr.set_layouts[2];
  VkDescriptorUpdateTemplate update_template =
      g_default_pipeline_layouts.pbr.update_templates[2];

  {
    VkDescriptorSetLayout set_layouts[ARRAY_SIZE(model->descriptor_sets)];
    for (size_t i = 0; i < ARRAY_SIZE(model->descriptor_sets); i++) {
      set_layouts[i] = set_layout;
    }

    VK_CHECK(vkAllocateDescriptorSets(
        g_ctx.device,
        &(VkDescriptorSetAllocateInfo){
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = g_ctx.descriptor_pool,
            .descriptorSetCount = ARRAY_SIZE(model->descriptor_sets),
            .pSetLayouts = set_layouts,
        },
        model->descriptor_sets));
  }

  for (uint32_t i = 0; i < RE_MAX_FRAMES_IN_FLIGHT; i++) {
    re_buffer_init(
        &model->buffers[i],
        &(re_buffer_options_t){
            .type = RE_BUFFER_TYPE_UNIFORM,
            .size = sizeof(model->uniform),
        });
    re_buffer_map_memory(&model->buffers[i], &model->mappings[i]);

    vkUpdateDescriptorSetWithTemplate(
        g_ctx.device,
        model->descriptor_sets[i],
        update_template,
        (re_descriptor_update_info_t[]){
            {.buffer_info = {.buffer = model->buffers[i].buffer,
                             .offset = 0,
                             .range = sizeof(model->uniform)}},
        });
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
