#include "gltf_model_component.h"
#include "../assets/gltf_model_asset.h"
#include "../engine.h"
#include <fstd_util.h>
#include <renderer/context.h>
#include <renderer/pipeline.h>
#include <renderer/util.h>
#include <renderer/window.h>
#include <string.h>

static void draw_node(
    eg_gltf_model_component_t *model,
    eg_gltf_model_asset_node_t *node,
    re_window_t *window,
    re_pipeline_t *pipeline) {
  VkCommandBuffer command_buffer = re_window_get_current_command_buffer(window);

  uint32_t i = window->current_frame;

  if (node->mesh != NULL) {
    vkCmdBindDescriptorSets(
        command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline->layout,
        2,
        1,
        &node->mesh->descriptor_sets[i],
        0,
        NULL);

    for (uint32_t j = 0; j < node->mesh->primitive_count; j++) {
      eg_gltf_model_asset_primitive_t *primitive = &node->mesh->primitives[j];

      if (primitive->material != NULL) {
        vkCmdPushConstants(
            command_buffer,
            pipeline->layout,
            VK_SHADER_STAGE_ALL_GRAPHICS,
            0,
            sizeof(primitive->material->uniform),
            &primitive->material->uniform);

        vkCmdBindDescriptorSets(
            command_buffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline->layout,
            4,
            1,
            &primitive->material->descriptor_sets[i],
            0,
            NULL);

        vkCmdDrawIndexed(
            command_buffer,
            primitive->index_count,
            1,
            primitive->first_index,
            0,
            0);
      }
    }
  }

  for (uint32_t j = 0; j < node->children_count; j++) {
    draw_node(model, node->children[j], window, pipeline);
  }
}

static void draw_node_picking(
    eg_gltf_model_component_t *model,
    eg_gltf_model_asset_node_t *node,
    re_window_t *window,
    VkCommandBuffer command_buffer,
    re_pipeline_t *pipeline) {
  uint32_t i = window->current_frame;

  if (node->mesh != NULL) {
    vkCmdBindDescriptorSets(
        command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline->layout,
        1,
        1,
        &node->mesh->descriptor_sets[i],
        0,
        NULL);

    for (uint32_t j = 0; j < node->mesh->primitive_count; j++) {
      eg_gltf_model_asset_primitive_t *primitive = &node->mesh->primitives[j];

      if (primitive->material != NULL) {
        vkCmdDrawIndexed(
            command_buffer,
            primitive->index_count,
            1,
            primitive->first_index,
            0,
            0);
      }
    }
  }

  for (uint32_t j = 0; j < node->children_count; j++) {
    draw_node_picking(
        model, node->children[j], window, command_buffer, pipeline);
  }
}

void eg_gltf_model_component_init(
    eg_gltf_model_component_t *model, eg_gltf_model_asset_t *asset) {
  model->asset = asset;

  model->ubo.matrix = mat4_identity();

  VkDescriptorSetLayout set_layouts[ARRAY_SIZE(model->descriptor_sets)];
  for (size_t i = 0; i < ARRAY_SIZE(model->descriptor_sets); i++) {
    set_layouts[i] = g_eng.set_layouts.model;
  }

  VkDescriptorSetAllocateInfo alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  alloc_info.pNext = NULL;
  alloc_info.descriptorPool = g_ctx.descriptor_pool;
  alloc_info.descriptorSetCount = ARRAY_SIZE(model->descriptor_sets);
  alloc_info.pSetLayouts = set_layouts;

  VK_CHECK(vkAllocateDescriptorSets(
      g_ctx.device, &alloc_info, model->descriptor_sets));

  for (uint32_t i = 0; i < RE_MAX_FRAMES_IN_FLIGHT; i++) {
    re_buffer_init(
        &model->uniform_buffers[i],
        &(re_buffer_options_t){
            .type = RE_BUFFER_TYPE_UNIFORM,
            .size = sizeof(model->ubo),
        });

    re_buffer_map_memory(&model->uniform_buffers[i], &model->mappings[i]);
    memcpy(model->mappings[i], &model->ubo, sizeof(model->ubo));

    VkDescriptorBufferInfo buffer_info = {
        model->uniform_buffers[i].buffer, 0, sizeof(model->ubo)};

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
  }
}

void eg_gltf_model_component_draw(
    eg_gltf_model_component_t *model,
    re_window_t *window,
    re_pipeline_t *pipeline,
    mat4_t transform) {
  VkCommandBuffer command_buffer = re_window_get_current_command_buffer(window);

  uint32_t i = window->current_frame;

  model->ubo.matrix = transform;
  memcpy(model->mappings[i], &model->ubo, sizeof(model->ubo));

  vkCmdBindPipeline(
      command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);

  VkDeviceSize offset = 0;
  VkBuffer vertex_buffer = model->asset->vertex_buffer.buffer;
  vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, &offset);

  vkCmdBindIndexBuffer(
      command_buffer,
      model->asset->index_buffer.buffer,
      0,
      VK_INDEX_TYPE_UINT32);

  vkCmdBindDescriptorSets(
      command_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline->layout,
      3,
      1,
      &model->descriptor_sets[i],
      0,
      NULL);

  for (uint32_t j = 0; j < model->asset->node_count; j++) {
    draw_node(model, &model->asset->nodes[j], window, pipeline);
  }
}

void eg_gltf_model_component_draw_picking(
    eg_gltf_model_component_t *model,
    re_window_t *window,
    VkCommandBuffer command_buffer,
    re_pipeline_t *pipeline,
    mat4_t transform) {
  uint32_t i = window->current_frame;

  model->ubo.matrix = transform;
  memcpy(model->mappings[i], &model->ubo, sizeof(model->ubo));

  vkCmdBindPipeline(
      command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);

  VkDeviceSize offset = 0;
  VkBuffer vertex_buffer = model->asset->vertex_buffer.buffer;
  vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, &offset);

  vkCmdBindIndexBuffer(
      command_buffer,
      model->asset->index_buffer.buffer,
      0,
      VK_INDEX_TYPE_UINT32);

  vkCmdBindDescriptorSets(
      command_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline->layout,
      2,
      1,
      &model->descriptor_sets[i],
      0,
      NULL);

  for (uint32_t j = 0; j < model->asset->node_count; j++) {
    draw_node_picking(
        model, &model->asset->nodes[j], window, command_buffer, pipeline);
  }
}

void eg_gltf_model_component_destroy(eg_gltf_model_component_t *model) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));
  for (uint32_t i = 0; i < RE_MAX_FRAMES_IN_FLIGHT; i++) {
    re_buffer_unmap_memory(&model->uniform_buffers[i]);
    re_buffer_destroy(&model->uniform_buffers[i]);
  }

  if (model->descriptor_sets[0] != VK_NULL_HANDLE) {
    vkFreeDescriptorSets(
        g_ctx.device,
        g_ctx.descriptor_pool,
        ARRAY_SIZE(model->descriptor_sets),
        model->descriptor_sets);
  }
}
