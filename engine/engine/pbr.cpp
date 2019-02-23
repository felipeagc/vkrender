#include "pbr.hpp"
#include <renderer/context.hpp>
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
