#include "picking_system.h"

#include "../pipelines.h"
#include <engine/components/gltf_model_component.h>
#include <engine/components/transform_component.h>
#include <renderer/context.h>
#include <renderer/util.h>
#include <renderer/window.h>
#include <string.h>

void eg_picking_system_init(
    eg_picking_system_t *system, uint32_t width, uint32_t height) {
  re_canvas_init(&system->canvas, width, height, VK_FORMAT_R32_UINT);
  system->canvas.clear_color = (VkClearColorValue){
      .uint32 = {UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX},
  };

  eg_init_pipeline_spv(
      &system->picking_pipeline,
      &system->canvas.render_target,
      "/shaders/picking.vert.spv",
      "/shaders/picking.frag.spv",
      eg_picking_pipeline_parameters());
}

void eg_picking_system_destroy(eg_picking_system_t *system) {
  re_pipeline_destroy(&system->picking_pipeline);
  re_canvas_destroy(&system->canvas);
}

void eg_picking_system_resize(
    eg_picking_system_t *system, uint32_t width, uint32_t height) {
  re_canvas_resize(&system->canvas, width, height);
}

eg_entity_t eg_picking_system_pick(
    eg_picking_system_t *system,
    uint32_t frame_index,
    eg_world_t *world,
    uint32_t mouse_x,
    uint32_t mouse_y) {

  VkCommandBuffer command_buffer;

  VkFence fence;

  // Create fence
  {
    VkFenceCreateInfo fence_create_info = {0};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = NULL;
    fence_create_info.flags = 0;

    VK_CHECK(vkCreateFence(g_ctx.device, &fence_create_info, NULL, &fence));
  }

  // Create and begin command buffer
  {
    VkCommandBufferAllocateInfo allocate_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        NULL,
        g_ctx.graphics_command_pool,     // commandPool
        VK_COMMAND_BUFFER_LEVEL_PRIMARY, // level
        1,                               // commandBufferCount
    };

    VK_CHECK(vkAllocateCommandBuffers(
        g_ctx.device, &allocate_info, &command_buffer));

    VkCommandBufferBeginInfo command_buffer_begin_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        NULL,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, // flags
        NULL,                                        // pInheritanceInfo
    };

    VK_CHECK(vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info));
  }

  const eg_cmd_info_t cmd_info = {
      .frame_index = frame_index,
      .cmd_buffer = command_buffer,
  };

  re_canvas_begin(&system->canvas, command_buffer);

  eg_camera_bind(
      &world->camera, &cmd_info, &system->picking_pipeline, 0);

  for (eg_entity_t entity = 0; entity < EG_MAX_ENTITIES; entity++) {
    if (eg_world_has_comp(world, entity, EG_GLTF_MODEL_COMPONENT_TYPE) &&
        eg_world_has_comp(world, entity, EG_TRANSFORM_COMPONENT_TYPE)) {
      eg_gltf_model_component_t *model =
          EG_GET_COMP(world, entity, eg_gltf_model_component_t);
      eg_transform_component_t *transform =
          EG_GET_COMP(world, entity, eg_transform_component_t);

      vkCmdPushConstants(
          command_buffer,
          system->picking_pipeline.layout,
          VK_SHADER_STAGE_ALL_GRAPHICS,
          0,
          sizeof(uint32_t),
          &entity);

      eg_gltf_model_component_draw_picking(
          model,
          &cmd_info,
          &system->picking_pipeline,
          eg_transform_component_to_mat4(transform));
    }
  }

  re_canvas_end(&system->canvas, command_buffer);

  // End and free command buffer
  {
    VK_CHECK(vkEndCommandBuffer(command_buffer));

    VkSubmitInfo submit_info = {
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        NULL,
        0,               // waitSemaphoreCount
        NULL,            // pWaitSemaphores
        NULL,            // pWaitDstStageMask
        1,               // commandBufferCount
        &command_buffer, // pCommandBuffers
        0,               // signalSemaphoreCount
        NULL,            // pSignalSemaphores
    };

    VK_CHECK(vkQueueSubmit(g_ctx.graphics_queue, 1, &submit_info, fence));

    VK_CHECK(vkWaitForFences(g_ctx.device, 1, &fence, VK_TRUE, UINT64_MAX));

    vkFreeCommandBuffers(
        g_ctx.device, g_ctx.graphics_command_pool, 1, &command_buffer);
  }

  // Destroy fence
  vkDestroyFence(g_ctx.device, fence, NULL);

  re_buffer_t staging_buffer;
  re_buffer_init(
      &staging_buffer,
      &(re_buffer_options_t){
          .type = RE_BUFFER_TYPE_TRANSFER,
          .size = sizeof(uint32_t),
      });

  re_image_transfer_to_buffer(
      system->canvas.resources[0].color.image,
      &staging_buffer,
      mouse_x,
      mouse_y,
      1,
      1,
      0,
      0);

  void *mapping;
  re_buffer_map_memory(&staging_buffer, &mapping);

  uint32_t entity;
  memcpy(&entity, mapping, sizeof(uint32_t));

  re_buffer_unmap_memory(&staging_buffer);

  re_buffer_destroy(&staging_buffer);

  return entity;
}
