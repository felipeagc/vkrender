#include "gltf_model_comp.h"
#include "../assets/gltf_model_asset.h"
#include "../pipelines.h"
#include <fstd_util.h>
#include <renderer/context.h>
#include <renderer/pipeline.h>
#include <renderer/util.h>
#include <renderer/window.h>
#include <string.h>

static void draw_node(
    eg_gltf_model_comp_t *model,
    eg_gltf_model_asset_node_t *node,
    const eg_cmd_info_t *cmd_info,
    re_pipeline_t *pipeline) {
  if (node->mesh != NULL) {
    vkCmdBindDescriptorSets(
        cmd_info->cmd_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline->layout.layout,
        2,
        1,
        &node->mesh->descriptor_sets[cmd_info->frame_index],
        0,
        NULL);

    for (uint32_t j = 0; j < node->mesh->primitive_count; j++) {
      eg_gltf_model_asset_primitive_t *primitive = &node->mesh->primitives[j];

      if (primitive->material != NULL) {
        vkCmdBindDescriptorSets(
            cmd_info->cmd_buffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline->layout.layout,
            4,
            1,
            &primitive->material->descriptor_sets[cmd_info->frame_index],
            0,
            NULL);

        vkCmdDrawIndexed(
            cmd_info->cmd_buffer,
            primitive->index_count,
            1,
            primitive->first_index,
            0,
            0);
      }
    }
  }

  for (uint32_t j = 0; j < node->children_count; j++) {
    draw_node(model, node->children[j], cmd_info, pipeline);
  }
}

static void draw_node_no_mat(
    eg_gltf_model_comp_t *model,
    eg_gltf_model_asset_node_t *node,
    const eg_cmd_info_t *cmd_info,
    re_pipeline_t *pipeline) {
  if (node->mesh != NULL) {
    vkCmdBindDescriptorSets(
        cmd_info->cmd_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline->layout.layout,
        1,
        1,
        &node->mesh->descriptor_sets[cmd_info->frame_index],
        0,
        NULL);

    for (uint32_t j = 0; j < node->mesh->primitive_count; j++) {
      eg_gltf_model_asset_primitive_t *primitive = &node->mesh->primitives[j];

      if (primitive->material != NULL) {
        vkCmdDrawIndexed(
            cmd_info->cmd_buffer,
            primitive->index_count,
            1,
            primitive->first_index,
            0,
            0);
      }
    }
  }

  for (uint32_t j = 0; j < node->children_count; j++) {
    draw_node_no_mat(model, node->children[j], cmd_info, pipeline);
  }
}

void eg_gltf_model_comp_init(
    eg_gltf_model_comp_t *model, eg_gltf_model_asset_t *asset) {
  model->asset = asset;

  model->ubo.matrix = mat4_identity();

  VkDescriptorSetLayout set_layout =
      g_default_pipeline_layouts.pbr.set_layouts[2];
  VkDescriptorUpdateTemplate update_template =
      g_default_pipeline_layouts.pbr.update_templates[2];

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

  for (uint32_t i = 0; i < RE_MAX_FRAMES_IN_FLIGHT; i++) {
    re_buffer_init(
        &model->uniform_buffers[i],
        &(re_buffer_options_t){
            .type = RE_BUFFER_TYPE_UNIFORM,
            .size = sizeof(model->ubo),
        });

    re_buffer_map_memory(&model->uniform_buffers[i], &model->mappings[i]);
    memcpy(model->mappings[i], &model->ubo, sizeof(model->ubo));

    vkUpdateDescriptorSetWithTemplate(
        g_ctx.device,
        model->descriptor_sets[i],
        update_template,
        (re_descriptor_update_info_t[]){
            {.buffer_info = {.buffer = model->uniform_buffers[i].buffer,
                             .offset = 0,
                             .range = sizeof(model->ubo)}},
        });
  }
}

void eg_gltf_model_comp_draw(
    eg_gltf_model_comp_t *model,
    const eg_cmd_info_t *cmd_info,
    re_pipeline_t *pipeline,
    mat4_t transform) {
  model->ubo.matrix = transform;
  memcpy(
      model->mappings[cmd_info->frame_index], &model->ubo, sizeof(model->ubo));

  eg_gltf_model_asset_update(model->asset, cmd_info);

  vkCmdBindPipeline(
      cmd_info->cmd_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline->pipeline);

  VkDeviceSize offset = 0;
  VkBuffer vertex_buffer = model->asset->vertex_buffer.buffer;
  vkCmdBindVertexBuffers(cmd_info->cmd_buffer, 0, 1, &vertex_buffer, &offset);

  vkCmdBindIndexBuffer(
      cmd_info->cmd_buffer,
      model->asset->index_buffer.buffer,
      0,
      VK_INDEX_TYPE_UINT32);

  vkCmdBindDescriptorSets(
      cmd_info->cmd_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline->layout.layout,
      3,
      1,
      &model->descriptor_sets[cmd_info->frame_index],
      0,
      NULL);

  for (uint32_t j = 0; j < model->asset->node_count; j++) {
    draw_node(model, &model->asset->nodes[j], cmd_info, pipeline);
  }
}

void eg_gltf_model_comp_draw_no_mat(
    eg_gltf_model_comp_t *model,
    const eg_cmd_info_t *cmd_info,
    re_pipeline_t *pipeline,
    mat4_t transform) {
  model->ubo.matrix = transform;
  memcpy(
      model->mappings[cmd_info->frame_index], &model->ubo, sizeof(model->ubo));

  eg_gltf_model_asset_update(model->asset, cmd_info);

  VkDeviceSize offset = 0;
  VkBuffer vertex_buffer = model->asset->vertex_buffer.buffer;
  vkCmdBindVertexBuffers(cmd_info->cmd_buffer, 0, 1, &vertex_buffer, &offset);

  vkCmdBindIndexBuffer(
      cmd_info->cmd_buffer,
      model->asset->index_buffer.buffer,
      0,
      VK_INDEX_TYPE_UINT32);

  vkCmdBindDescriptorSets(
      cmd_info->cmd_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline->layout.layout,
      2,
      1,
      &model->descriptor_sets[cmd_info->frame_index],
      0,
      NULL);

  for (uint32_t j = 0; j < model->asset->node_count; j++) {
    draw_node_no_mat(model, &model->asset->nodes[j], cmd_info, pipeline);
  }
}

void eg_gltf_model_comp_destroy(eg_gltf_model_comp_t *model) {
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
