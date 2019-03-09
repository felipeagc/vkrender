#include "pbr.h"
#include "engine.h"
#include <fstd_util.h>
#include <renderer/context.h>
#include <renderer/pipeline.h>
#include <renderer/texture.h>
#include <renderer/util.h>
#include <renderer/window.h>

void eg_pbr_model_init(eg_pbr_model_t *model, mat4_t transform) {
  model->uniform.transform = transform;

  {
    VkDescriptorSetLayout set_layouts[ARRAYSIZE(model->descriptor_sets)];
    for (size_t i = 0; i < ARRAYSIZE(model->descriptor_sets); i++) {
      set_layouts[i] = g_eng.set_layouts.model;
    }

    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.descriptorPool = g_ctx.descriptor_pool;
    alloc_info.descriptorSetCount = ARRAYSIZE(model->descriptor_sets);
    alloc_info.pSetLayouts = set_layouts;

    VK_CHECK(vkAllocateDescriptorSets(
        g_ctx.device, &alloc_info, model->descriptor_sets));
  }

  for (uint32_t i = 0; i < RE_MAX_FRAMES_IN_FLIGHT; i++) {
    re_buffer_init_uniform(&model->buffers[i], sizeof(model->uniform));

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
        NULL,                           // pImageInfo
        &buffer_info,                      // pBufferInfo
        NULL,                           // pTexelBufferView
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
      &model->descriptor_sets[i],
      0,
      NULL);
}

void eg_pbr_model_destroy(eg_pbr_model_t *model) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  vkFreeDescriptorSets(
      g_ctx.device,
      g_ctx.descriptor_pool,
      ARRAYSIZE(model->descriptor_sets),
      model->descriptor_sets);

  for (uint32_t i = 0; i < RE_MAX_FRAMES_IN_FLIGHT; i++) {
    re_buffer_unmap_memory(&model->buffers[i]);
    re_buffer_destroy(&model->buffers[i]);
  }
}
