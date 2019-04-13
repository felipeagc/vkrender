#include "picking_system.h"

#include "../pipelines.h"
#include <engine/components/gltf_model_component.h>
#include <engine/components/transform_component.h>
#include <renderer/context.h>
#include <renderer/util.h>
#include <renderer/window.h>
#include <string.h>

void eg_picking_system_init(
    eg_picking_system_t *system,
    re_render_target_t *render_target,
    uint32_t width,
    uint32_t height) {
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

  eg_init_pipeline_spv(
      &system->gizmo_pipeline,
      render_target,
      "/shaders/gizmo.vert.spv",
      "/shaders/gizmo.frag.spv",
      eg_gizmo_pipeline_parameters());

  const float thick = 0.05f;

  const re_vertex_t pos_gizmo_vertices[] = {
      (re_vertex_t){.pos = (vec3_t){thick, thick, thick}},
      (re_vertex_t){.pos = (vec3_t){thick, -thick, thick}},
      (re_vertex_t){.pos = (vec3_t){thick, -thick, -thick}},
      (re_vertex_t){.pos = (vec3_t){thick, thick, -thick}},

      (re_vertex_t){.pos = (vec3_t){thick * 20.0f, thick, thick}},
      (re_vertex_t){.pos = (vec3_t){thick * 20.0f, -thick, thick}},
      (re_vertex_t){.pos = (vec3_t){thick * 20.0f, -thick, -thick}},
      (re_vertex_t){.pos = (vec3_t){thick * 20.0f, thick, -thick}},
  };

  // clang-format off
  const uint32_t pos_gizmo_indices[] = {
    1, 2, 3,
    3, 0, 1,

    1, 0, 4,
    4, 5, 1,

    6, 7, 3,
    3, 2, 6,

    0, 3, 7,
    7, 4, 0,

    5, 6, 2,
    2, 1, 5,

    5, 4, 7,
    7, 6, 5,
  };
  // clang-format on

  system->pos_gizmo_index_count = ARRAY_SIZE(pos_gizmo_indices);

  re_buffer_init(
      &system->pos_gizmo_vertex_buffer,
      &(re_buffer_options_t){
          .type = RE_BUFFER_TYPE_VERTEX,
          .size = sizeof(pos_gizmo_vertices),
      });
  re_buffer_init(
      &system->pos_gizmo_index_buffer,
      &(re_buffer_options_t){
          .type = RE_BUFFER_TYPE_INDEX,
          .size = sizeof(pos_gizmo_indices),
      });

  size_t staging_size = sizeof(pos_gizmo_vertices) > sizeof(pos_gizmo_indices)
                            ? sizeof(pos_gizmo_vertices)
                            : sizeof(pos_gizmo_indices);

  re_buffer_t staging_buffer;
  re_buffer_init(
      &staging_buffer,
      &(re_buffer_options_t){
          .type = RE_BUFFER_TYPE_TRANSFER,
          .size = staging_size,
      });

  void *memory;
  re_buffer_map_memory(&staging_buffer, &memory);

  memcpy(memory, pos_gizmo_vertices, sizeof(pos_gizmo_vertices));
  re_buffer_transfer_to_buffer(
      &staging_buffer,
      &system->pos_gizmo_vertex_buffer,
      g_ctx.transient_command_pool,
      sizeof(pos_gizmo_vertices));

  memcpy(memory, pos_gizmo_indices, sizeof(pos_gizmo_indices));
  re_buffer_transfer_to_buffer(
      &staging_buffer,
      &system->pos_gizmo_index_buffer,
      g_ctx.transient_command_pool,
      sizeof(pos_gizmo_indices));

  re_buffer_unmap_memory(&staging_buffer);

  re_buffer_destroy(&staging_buffer);
}

void eg_picking_system_destroy(eg_picking_system_t *system) {
  re_pipeline_destroy(&system->gizmo_pipeline);
  re_buffer_destroy(&system->pos_gizmo_vertex_buffer);
  re_buffer_destroy(&system->pos_gizmo_index_buffer);

  re_pipeline_destroy(&system->picking_pipeline);
  re_canvas_destroy(&system->canvas);
}

void eg_picking_system_resize(
    eg_picking_system_t *system, uint32_t width, uint32_t height) {
  re_canvas_resize(&system->canvas, width, height);
}

void eg_picking_system_draw_gizmos(
    eg_picking_system_t *system,
    eg_world_t *world,
    eg_entity_t entity,
    const eg_cmd_info_t *cmd_info,
    eg_camera_t *camera,
    uint32_t width,
    uint32_t height) {
  if (entity >= EG_MAX_ENTITIES) {
    return;
  }

  if (!eg_world_has_comp(world, entity, EG_TRANSFORM_COMPONENT_TYPE)) {
    return;
  }

  vkCmdClearAttachments(
      cmd_info->cmd_buffer,
      1,
      &(VkClearAttachment){
          .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
          .clearValue.depthStencil.depth = 1.0f,
          .clearValue.depthStencil.stencil = 0,
      },
      1,
      &(VkClearRect){
          .rect.offset.x = 0,
          .rect.offset.y = 0,
          .rect.extent.width = width,
          .rect.extent.height = height,
          .baseArrayLayer = 0,
          .layerCount = 1,
      });

  re_cmd_bind_graphics_pipeline(cmd_info->cmd_buffer, &system->gizmo_pipeline);

  vkCmdBindIndexBuffer(
      cmd_info->cmd_buffer,
      system->pos_gizmo_index_buffer.buffer,
      0,
      VK_INDEX_TYPE_UINT32);
  VkDeviceSize offsets = 0;
  vkCmdBindVertexBuffers(
      cmd_info->cmd_buffer,
      0,
      1,
      &system->pos_gizmo_vertex_buffer.buffer,
      &offsets);

  struct {
    mat4_t mvp;
    vec4_t color;
  } push_constant;

  eg_transform_component_t *transform =
      EG_GET_COMP(world, entity, eg_transform_component_t);

  mat4_t object_mat = eg_transform_component_to_mat4(transform);
  mat4_t mats[] = {
      mat4_identity(),
      mat4_identity(),
      mat4_identity(),
  };

  // Y axis
  mats[1].v[0] = (vec4_t){0.0f, 1.0f, 0.0f, 0.0f};
  mats[1].v[1] = (vec4_t){1.0f, 0.0f, 0.0f, 0.0f};

  // Z axis
  mats[2].v[0] = (vec4_t){0.0f, 0.0f, 1.0f, 0.0f};
  mats[2].v[2] = (vec4_t){1.0f, 0.0f, 0.0f, 0.0f};

  vec4_t colors[] = {
      (vec4_t){1.0f, 0.0f, 0.0f, 0.5f},
      (vec4_t){0.0f, 1.0f, 0.0f, 0.5f},
      (vec4_t){0.0f, 0.0f, 1.0f, 0.5f},
  };

  for (uint32_t i = 0; i < 3; i++) {
    mats[i].columns[3][0] = object_mat.columns[3][0];
    mats[i].columns[3][1] = object_mat.columns[3][1];
    mats[i].columns[3][2] = object_mat.columns[3][2];

    push_constant.mvp =
        mat4_mul(mats[i], mat4_mul(camera->uniform.view, camera->uniform.proj));
    push_constant.color = colors[i];

    vkCmdPushConstants(
        cmd_info->cmd_buffer,
        system->gizmo_pipeline.layout,
        VK_SHADER_STAGE_ALL_GRAPHICS,
        0,
        sizeof(push_constant),
        &push_constant);

    vkCmdDrawIndexed(
        cmd_info->cmd_buffer, system->pos_gizmo_index_count, 1, 0, 0, 0);
  }
}

eg_entity_t eg_picking_system_pick(
    eg_picking_system_t *system,
    uint32_t frame_index,
    eg_world_t *world,
    uint32_t mouse_x,
    uint32_t mouse_y) {

  re_cmd_buffer_t command_buffer;

  VkFence fence;

  // Create fence
  {
    VkFenceCreateInfo fence_create_info = {0};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = NULL;
    fence_create_info.flags = 0;

    VK_CHECK(vkCreateFence(g_ctx.device, &fence_create_info, NULL, &fence));
  }

  re_allocate_cmd_buffers(
      &(re_cmd_buffer_alloc_info_t){
          .pool = g_ctx.graphics_command_pool,
          .count = 1,
          .level = RE_CMD_BUFFER_LEVEL_PRIMARY,
      },
      &command_buffer);

  re_begin_cmd_buffer(
      command_buffer,
      &(re_cmd_buffer_begin_info_t){
          .usage = RE_CMD_BUFFER_USAGE_ONE_TIME_SUBMIT,
      });

  const eg_cmd_info_t cmd_info = {
      .frame_index = frame_index,
      .cmd_buffer = command_buffer,
  };

  re_canvas_begin(&system->canvas, command_buffer);

  eg_camera_bind(&world->camera, &cmd_info, &system->picking_pipeline, 0);

  for (eg_entity_t entity = 0; entity < EG_MAX_ENTITIES; entity++) {
    if (eg_world_has_tag(world, entity, EG_ENTITY_TAG_HIDDEN)) {
      continue;
    }

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
    re_end_cmd_buffer(command_buffer);

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

    re_free_cmd_buffers(g_ctx.graphics_command_pool, 1, &command_buffer);
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
      g_ctx.transient_command_pool,
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