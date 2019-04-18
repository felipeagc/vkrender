#include "inspector.h"
#include "asset_manager.h"
#include "assets/environment_asset.h"
#include "assets/gltf_model_asset.h"
#include "assets/mesh_asset.h"
#include "assets/pbr_material_asset.h"
#include "components/gltf_model_component.h"
#include "components/mesh_component.h"
#include "components/transform_component.h"
#include "imgui.h"
#include "pipelines.h"
#include "world.h"
#include <renderer/context.h>
#include <renderer/window.h>
#include <stdio.h>

#define INDENT_LEVEL 8.0f

// clang-format off
static const uint32_t pos_gizmo_indices[] = {
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

static const float thickness = 0.05f;

static const re_vertex_t pos_gizmo_vertices[] = {
    (re_vertex_t){.pos = (vec3_t){thickness, thickness, thickness}},
    (re_vertex_t){.pos = (vec3_t){thickness, -thickness, thickness}},
    (re_vertex_t){.pos = (vec3_t){thickness, -thickness, -thickness}},
    (re_vertex_t){.pos = (vec3_t){thickness, thickness, -thickness}},

    (re_vertex_t){.pos = (vec3_t){thickness * 20.0f, thickness, thickness}},
    (re_vertex_t){.pos = (vec3_t){thickness * 20.0f, -thickness, thickness}},
    (re_vertex_t){.pos = (vec3_t){thickness * 20.0f, -thickness, -thickness}},
    (re_vertex_t){.pos = (vec3_t){thickness * 20.0f, thickness, -thickness}},
};

static void inspector_camera(eg_camera_t *camera) {
  float deg = to_degrees(camera->fov);
  igDragFloat("FOV", &deg, 0.1f, 0.0f, 0.0f, "%.3f", 1.0f);
  camera->fov = to_radians(deg);
}

static void inspector_environment(eg_environment_t *environment) {
  igDragFloat3(
      "Sun direction",
      &environment->uniform.sun_direction.x,
      0.01f,
      0.0f,
      0.0f,
      "%.3f",
      1.0f);

  igColorEdit3("Sun color", &environment->uniform.sun_color.x, 0);

  igDragFloat(
      "Sun intensity",
      &environment->uniform.sun_intensity,
      0.01f,
      0.0f,
      0.0f,
      "%.3f",
      1.0f);

  igDragFloat(
      "Exposure",
      &environment->uniform.exposure,
      0.01f,
      0.0f,
      0.0f,
      "%.3f",
      1.0f);
}

static void inspector_statistics(re_window_t *window) {
  igText("Delta time: %.4fms", window->delta_time);
  igText("FPS: %.2f", 1.0f / window->delta_time);
}

static void inspector_environment_asset(eg_asset_t *asset) {}

static void inspector_pbr_material_asset(eg_asset_t *asset) {
  eg_pbr_material_asset_t *material = (eg_pbr_material_asset_t *)asset;

  igColorEdit4("Color", &material->uniform.base_color_factor.x, 0);
  igDragFloat(
      "Metallic", &material->uniform.metallic, 0.01f, 0.0f, 1.0f, "%.3f", 1.0f);
  igDragFloat(
      "Roughness",
      &material->uniform.roughness,
      0.01f,
      0.0f,
      1.0f,
      "%.3f",
      1.0f);
  igColorEdit4("Emissive factor", &material->uniform.emissive_factor.x, 0);
}

static void inspector_gltf_model_asset(eg_asset_t *asset) {
  eg_gltf_model_asset_t *gltf_asset = (eg_gltf_model_asset_t *)asset;

  igText("Vertex count: %u", gltf_asset->vertex_count);
  igText("Index count: %u", gltf_asset->index_count);
  igText("Image count: %u", gltf_asset->image_count);
  igText("Mesh count: %u", gltf_asset->mesh_count);

  for (uint32_t j = 0; j < gltf_asset->material_count; j++) {
    igPushIDInt(j);
    eg_gltf_model_asset_material_t *material = &gltf_asset->materials[j];

    if (igCollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
      igColorEdit4("Color", &material->uniform.base_color_factor.x, 0);
      igDragFloat(
          "Metallic",
          &material->uniform.metallic,
          0.01f,
          0.0f,
          1.0f,
          "%.3f",
          1.0f);
      igDragFloat(
          "Roughness",
          &material->uniform.roughness,
          0.01f,
          0.0f,
          1.0f,
          "%.3f",
          1.0f);
      igColorEdit4("Emissive factor", &material->uniform.emissive_factor.x, 0);
    }
    igPopID();
  }
}

static void
inspector_transform_component(eg_world_t *world, eg_entity_t entity) {
  eg_transform_component_t *transform =
      EG_GET_COMP(world, entity, eg_transform_component_t);

  igDragFloat3(
      "Position", &transform->position.x, 0.1f, 0.0f, 0.0f, "%.3f", 1.0f);
  igDragFloat3("Scale", &transform->scale.x, 0.1f, 0.0f, 0.0f, "%.3f", 1.0f);
  igDragFloat3("Axis", &transform->axis.x, 0.01f, 0.0f, 1.0f, "%.3f", 1.0f);
  igDragFloat("Angle", &transform->angle, 0.01f, 0.0f, 0.0f, "%.3f rad", 1.0f);
}

static void
inspector_gltf_model_component(eg_world_t *world, eg_entity_t entity) {
  eg_gltf_model_component_t *gltf_model =
      EG_GET_COMP(world, entity, eg_gltf_model_component_t);

  igText("Asset: %s", gltf_model->asset->asset.name);
  igSameLine(0.0f, -1.0f);
  if (igSmallButton("Inspect")) {
    igOpenPopup("gltfmodelpopup");
  }

  if (igBeginPopup("gltfmodelpopup", 0)) {
    inspector_gltf_model_asset((eg_asset_t *)gltf_model->asset);
    igEndPopup();
  }
}

static void inspector_mesh_component(eg_world_t *world, eg_entity_t entity) {}

void eg_inspector_init(
    eg_inspector_t *inspector,
    re_window_t *window,
    re_render_target_t *render_target,
    eg_world_t *world,
    eg_asset_manager_t *asset_manager) {
  inspector->selected_entity = UINT32_MAX;
  inspector->window = window;
  inspector->world = world;
  inspector->asset_manager = asset_manager;
  inspector->drawing_render_target = render_target;

  re_canvas_init(
      &inspector->canvas,
      inspector->drawing_render_target->width,
      inspector->drawing_render_target->height,
      VK_FORMAT_R32_UINT);
  inspector->canvas.clear_color = (VkClearColorValue){
      .uint32 = {UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX},
  };

  eg_init_pipeline_spv(
      &inspector->gizmo_pipeline,
      inspector->drawing_render_target,
      "/shaders/gizmo.vert.spv",
      "/shaders/gizmo.frag.spv",
      eg_gizmo_pipeline_parameters());

  eg_init_pipeline_spv(
      &inspector->gizmo_picking_pipeline,
      &inspector->canvas.render_target,
      "/shaders/gizmo_picking.vert.spv",
      "/shaders/gizmo_picking.frag.spv",
      eg_gizmo_pipeline_parameters());

  eg_init_pipeline_spv(
      &inspector->picking_pipeline,
      &inspector->canvas.render_target,
      "/shaders/picking.vert.spv",
      "/shaders/picking.frag.spv",
      eg_picking_pipeline_parameters());

  inspector->drag_direction = EG_DRAG_DIRECTION_NONE;
  inspector->pos_delta = (vec3_t){0.0f, 0.0f, 0.0f};

  inspector->pos_gizmo_index_count = ARRAY_SIZE(pos_gizmo_indices);

  re_buffer_init(
      &inspector->pos_gizmo_vertex_buffer,
      &(re_buffer_options_t){
          .type = RE_BUFFER_TYPE_VERTEX,
          .size = sizeof(pos_gizmo_vertices),
      });
  re_buffer_init(
      &inspector->pos_gizmo_index_buffer,
      &(re_buffer_options_t){
          .type = RE_BUFFER_TYPE_INDEX,
          .size = sizeof(pos_gizmo_indices),
      });

  size_t staging_size =
      MAX(sizeof(pos_gizmo_vertices), sizeof(pos_gizmo_indices));

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
      &inspector->pos_gizmo_vertex_buffer,
      g_ctx.transient_command_pool,
      sizeof(pos_gizmo_vertices));

  memcpy(memory, pos_gizmo_indices, sizeof(pos_gizmo_indices));
  re_buffer_transfer_to_buffer(
      &staging_buffer,
      &inspector->pos_gizmo_index_buffer,
      g_ctx.transient_command_pool,
      sizeof(pos_gizmo_indices));

  re_buffer_unmap_memory(&staging_buffer);

  re_buffer_destroy(&staging_buffer);
}

void eg_inspector_destroy(eg_inspector_t *inspector) {
  re_pipeline_destroy(&inspector->gizmo_pipeline);
  re_pipeline_destroy(&inspector->gizmo_picking_pipeline);
  re_buffer_destroy(&inspector->pos_gizmo_vertex_buffer);
  re_buffer_destroy(&inspector->pos_gizmo_index_buffer);

  re_pipeline_destroy(&inspector->picking_pipeline);
  re_canvas_destroy(&inspector->canvas);
}

/*
 *
 * Event callbacks
 *
 */

static void framebuffer_resized(
    eg_inspector_t *inspector, uint32_t width, uint32_t height) {
  re_canvas_resize(&inspector->canvas, width, height);
}

static void
draw_gizmos_picking(eg_inspector_t *inspector, const eg_cmd_info_t *cmd_info) {
  if (inspector->selected_entity >= EG_MAX_ENTITIES) {
    return;
  }

  if (!eg_world_has_comp(
          inspector->world,
          inspector->selected_entity,
          EG_TRANSFORM_COMPONENT_TYPE)) {
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
          .rect.extent.width = inspector->canvas.render_target.width,
          .rect.extent.height = inspector->canvas.render_target.height,
          .baseArrayLayer = 0,
          .layerCount = 1,
      });

  re_cmd_bind_graphics_pipeline(
      cmd_info->cmd_buffer, &inspector->gizmo_picking_pipeline);

  vkCmdBindIndexBuffer(
      cmd_info->cmd_buffer,
      inspector->pos_gizmo_index_buffer.buffer,
      0,
      VK_INDEX_TYPE_UINT32);
  VkDeviceSize offsets = 0;
  vkCmdBindVertexBuffers(
      cmd_info->cmd_buffer,
      0,
      1,
      &inspector->pos_gizmo_vertex_buffer.buffer,
      &offsets);

  struct {
    mat4_t mvp;
    uint32_t index;
  } push_constant;

  eg_transform_component_t *transform = EG_GET_COMP(
      inspector->world, inspector->selected_entity, eg_transform_component_t);

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

  uint32_t color_indices[] = {
      EG_DRAG_DIRECTION_X,
      EG_DRAG_DIRECTION_Y,
      EG_DRAG_DIRECTION_Z,
  };

  for (uint32_t i = 0; i < 3; i++) {
    mats[i].cols[3][0] = object_mat.cols[3][0];
    mats[i].cols[3][1] = object_mat.cols[3][1];
    mats[i].cols[3][2] = object_mat.cols[3][2];

    push_constant.mvp = mat4_mul(
        mats[i],
        mat4_mul(
            inspector->world->camera.uniform.view,
            inspector->world->camera.uniform.proj));
    push_constant.index = color_indices[i];

    vkCmdPushConstants(
        cmd_info->cmd_buffer,
        inspector->gizmo_picking_pipeline.layout.layout,
        inspector->gizmo_picking_pipeline.layout.push_constants[0].stageFlags,
        inspector->gizmo_picking_pipeline.layout.push_constants[0].offset,
        sizeof(push_constant),
        &push_constant);

    vkCmdDrawIndexed(
        cmd_info->cmd_buffer, inspector->pos_gizmo_index_count, 1, 0, 0, 0);
  }
}

static void mouse_pressed(eg_inspector_t *inspector) {
  if (igIsWindowHovered(
          ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_ChildWindows |
          ImGuiHoveredFlags_AllowWhenBlockedByPopup |
          ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
    return;
  }

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
      .frame_index = inspector->window->current_frame,
      .cmd_buffer = command_buffer,
  };

  re_canvas_begin(&inspector->canvas, command_buffer);

  eg_camera_bind(
      &inspector->world->camera, &cmd_info, &inspector->picking_pipeline, 0);

  for (eg_entity_t entity = 0; entity < EG_MAX_ENTITIES; entity++) {
    if (eg_world_has_tag(inspector->world, entity, EG_ENTITY_TAG_HIDDEN)) {
      continue;
    }

    if (eg_world_has_comp(
            inspector->world, entity, EG_GLTF_MODEL_COMPONENT_TYPE) &&
        eg_world_has_comp(
            inspector->world, entity, EG_TRANSFORM_COMPONENT_TYPE)) {
      eg_gltf_model_component_t *model =
          EG_GET_COMP(inspector->world, entity, eg_gltf_model_component_t);
      eg_transform_component_t *transform =
          EG_GET_COMP(inspector->world, entity, eg_transform_component_t);

      vkCmdPushConstants(
          command_buffer,
          inspector->picking_pipeline.layout.layout,
          inspector->picking_pipeline.layout.push_constants[0].stageFlags,
          inspector->picking_pipeline.layout.push_constants[0].offset,
          sizeof(uint32_t),
          &entity);

      eg_gltf_model_component_draw_picking(
          model,
          &cmd_info,
          &inspector->picking_pipeline,
          eg_transform_component_to_mat4(transform));
    }
  }

  draw_gizmos_picking(inspector, &cmd_info);

  re_canvas_end(&inspector->canvas, command_buffer);

  // End and free command buffer
  {
    re_end_cmd_buffer(command_buffer);

    VK_CHECK(vkQueueSubmit(
        g_ctx.graphics_queue,
        1,
        &(VkSubmitInfo){
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = NULL,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = NULL,
            .pWaitDstStageMask = NULL,
            .commandBufferCount = 1,
            .pCommandBuffers = &command_buffer,
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = NULL,
        },
        fence));

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

  double cursor_x, cursor_y;
  re_window_get_cursor_pos(inspector->window, &cursor_x, &cursor_y);

  re_image_transfer_to_buffer(
      inspector->canvas.resources[0].color.image,
      &staging_buffer,
      g_ctx.transient_command_pool,
      (uint32_t)cursor_x,
      (uint32_t)cursor_y,
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

  switch (entity) {
  case EG_DRAG_DIRECTION_X:
  case EG_DRAG_DIRECTION_Y:
  case EG_DRAG_DIRECTION_Z: {
    inspector->drag_direction = entity;
    break;
  }
  default: {
    inspector->selected_entity = entity;
    break;
  }
  }

  if (inspector->selected_entity < EG_MAX_ENTITIES &&
      eg_world_has_comp(
          inspector->world,
          inspector->selected_entity,
          EG_TRANSFORM_COMPONENT_TYPE)) {
    eg_transform_component_t *transform = EG_GET_COMP(
        inspector->world, inspector->selected_entity, eg_transform_component_t);
    vec3_t transform_ndc =
        eg_camera_world_to_ndc(&inspector->world->camera, transform->position);

    float nx =
        (((float)cursor_x / (float)inspector->canvas.render_target.width) *
         2.0f) -
        1.0f;
    float ny =
        (((float)cursor_y / (float)inspector->canvas.render_target.height) *
         2.0f) -
        1.0f;
    vec3_t cursor_ndc = {nx, ny, transform_ndc.z};
    vec3_t cursor_world =
        eg_camera_ndc_to_world(&inspector->world->camera, cursor_ndc);

    inspector->pos_delta = vec3_sub(transform->position, cursor_world);
  }
}

static void mouse_released(eg_inspector_t *inspector) {
  inspector->pos_delta = (vec3_t){0.0f, 0.0f, 0.0f};
  inspector->drag_direction = EG_DRAG_DIRECTION_NONE;
}

void eg_inspector_process_event(
    eg_inspector_t *inspector, const re_event_t *event) {
  switch (event->type) {
  case RE_EVENT_FRAMEBUFFER_RESIZED: {
    framebuffer_resized(
        inspector, (uint32_t)event->size.width, (uint32_t)event->size.height);
    break;
  }
  case RE_EVENT_BUTTON_PRESSED: {
    if (event->mouse.button == GLFW_MOUSE_BUTTON_LEFT) {
      mouse_pressed(inspector);
    }
    break;
  }
  case RE_EVENT_BUTTON_RELEASED: {
    if (event->mouse.button == GLFW_MOUSE_BUTTON_LEFT) {
      mouse_released(inspector);
    }
    break;
  }
  default:
    break;
  }
}

void eg_inspector_update(eg_inspector_t *inspector) {
  if (inspector->drag_direction == EG_DRAG_DIRECTION_NONE ||
      inspector->selected_entity >= EG_MAX_ENTITIES ||
      !eg_world_has_comp(
          inspector->world,
          inspector->selected_entity,
          EG_TRANSFORM_COMPONENT_TYPE)) {
    return;
  }

  eg_transform_component_t *transform = EG_GET_COMP(
      inspector->world, inspector->selected_entity, eg_transform_component_t);
  vec3_t transform_ndc =
      eg_camera_world_to_ndc(&inspector->world->camera, transform->position);

  double cursor_x, cursor_y;
  re_window_get_cursor_pos(inspector->window, &cursor_x, &cursor_y);
  float nx =
      (((float)cursor_x / (float)inspector->drawing_render_target->width) *
       2.0f) -
      1.0f;
  float ny =
      (((float)cursor_y / (float)inspector->drawing_render_target->height) *
       2.0f) -
      1.0f;
  vec3_t cursor_ndc = {nx, ny, transform_ndc.z};
  vec3_t cursor_world =
      eg_camera_ndc_to_world(&inspector->world->camera, cursor_ndc);

  switch (inspector->drag_direction) {
  case EG_DRAG_DIRECTION_X: {
    transform->position.x = cursor_world.x + inspector->pos_delta.x;
    break;
  }
  case EG_DRAG_DIRECTION_Y: {
    transform->position.y = cursor_world.y + inspector->pos_delta.y;
    break;
  }
  case EG_DRAG_DIRECTION_Z: {
    transform->position.z = cursor_world.z + inspector->pos_delta.z;
    break;
  }
  default: {
    break;
  }
  }
}

void eg_inspector_draw_gizmos(
    eg_inspector_t *inspector, const eg_cmd_info_t *cmd_info) {
  if (inspector->selected_entity >= EG_MAX_ENTITIES) {
    return;
  }

  if (!eg_world_has_comp(
          inspector->world,
          inspector->selected_entity,
          EG_TRANSFORM_COMPONENT_TYPE)) {
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
          .rect.extent.width = inspector->drawing_render_target->width,
          .rect.extent.height = inspector->drawing_render_target->height,
          .baseArrayLayer = 0,
          .layerCount = 1,
      });

  re_cmd_bind_graphics_pipeline(
      cmd_info->cmd_buffer, &inspector->gizmo_pipeline);

  vkCmdBindIndexBuffer(
      cmd_info->cmd_buffer,
      inspector->pos_gizmo_index_buffer.buffer,
      0,
      VK_INDEX_TYPE_UINT32);
  VkDeviceSize offsets = 0;
  vkCmdBindVertexBuffers(
      cmd_info->cmd_buffer,
      0,
      1,
      &inspector->pos_gizmo_vertex_buffer.buffer,
      &offsets);

  struct {
    mat4_t mvp;
    vec4_t color;
  } push_constant;

  eg_transform_component_t *transform = EG_GET_COMP(
      inspector->world, inspector->selected_entity, eg_transform_component_t);

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
    mats[i].cols[3][0] = object_mat.cols[3][0];
    mats[i].cols[3][1] = object_mat.cols[3][1];
    mats[i].cols[3][2] = object_mat.cols[3][2];

    push_constant.mvp = mat4_mul(
        mats[i],
        mat4_mul(
            inspector->world->camera.uniform.view,
            inspector->world->camera.uniform.proj));
    push_constant.color = colors[i];

    vkCmdPushConstants(
        cmd_info->cmd_buffer,
        inspector->gizmo_pipeline.layout.layout,
        inspector->gizmo_pipeline.layout.push_constants[0].stageFlags,
        inspector->gizmo_pipeline.layout.push_constants[0].offset,
        sizeof(push_constant),
        &push_constant);

    vkCmdDrawIndexed(
        cmd_info->cmd_buffer, inspector->pos_gizmo_index_count, 1, 0, 0, 0);
  }
}

void eg_inspector_draw_ui(eg_inspector_t *inspector) {
  static char str[256] = "";

  re_window_t *window = inspector->window;
  eg_world_t *world = inspector->world;
  eg_asset_manager_t *asset_manager = inspector->asset_manager;

  if (inspector->selected_entity != UINT32_MAX) {
    if (igBegin("Selected entity", NULL, 0)) {
      eg_entity_t entity = inspector->selected_entity;

      igText("Entity #%u", inspector->selected_entity);

      if (eg_world_has_comp(world, entity, EG_TRANSFORM_COMPONENT_TYPE) &&
          igCollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        inspector_transform_component(world, entity);
      }

      if (eg_world_has_comp(world, entity, EG_MESH_COMPONENT_TYPE) &&
          igCollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen)) {
        inspector_mesh_component(world, entity);
      }

      if (eg_world_has_comp(world, entity, EG_GLTF_MODEL_COMPONENT_TYPE) &&
          igCollapsingHeader("GLTF Model", ImGuiTreeNodeFlags_DefaultOpen)) {
        inspector_gltf_model_component(world, entity);
      }
    }
    igEnd();
  }

  if (igBegin("Inspector", NULL, 0)) {
    if (igBeginTabBar("Inspector", 0)) {
      if (igBeginTabItem("World", NULL, 0)) {
        if (igCollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
          inspector_camera(&world->camera);
        }

        if (igCollapsingHeader("Environment", ImGuiTreeNodeFlags_DefaultOpen)) {
          inspector_environment(&world->environment);
        }

        igEndTabItem();
      }

      if (igBeginTabItem("Statistics", NULL, 0)) {
        inspector_statistics(window);
        igEndTabItem();
      }

      if (igBeginTabItem("Entities", NULL, 0)) {
        for (eg_entity_t entity = 0; entity < EG_MAX_ENTITIES; entity++) {
          if (!eg_world_has_any_comp(world, entity)) {
            continue;
          }

          snprintf(str, sizeof(str), "Entity #%d", entity);
          if (igSelectable(str, false, 0, (ImVec2){0.0f, 0.0f})) {
            inspector->selected_entity = entity;
          }
        }

        igEndTabItem();
      }

      if (igBeginTabItem("Assets", NULL, 0)) {
        for (uint32_t i = 0; i < EG_MAX_ASSETS; i++) {
          eg_asset_t *asset = eg_asset_manager_get_by_index(asset_manager, i);
          if (asset == NULL) {
            continue;
          }

          igPushIDInt(i);

#define ASSET_HEADER(format, ...)                                              \
  snprintf(str, sizeof(str), format, __VA_ARGS__);                             \
  if (igCollapsingHeader(str, 0))

          switch (asset->type) {
          case EG_ENVIRONMENT_ASSET_TYPE: {
            ASSET_HEADER("Environment: %s", asset->name) {
              inspector_environment_asset(asset);
            }
            break;
          }
          case EG_PBR_MATERIAL_ASSET_TYPE: {
            ASSET_HEADER("Material: %s", asset->name) {
              inspector_pbr_material_asset(asset);
            }
            break;
          }
          case EG_MESH_ASSET_TYPE: {
            ASSET_HEADER("Mesh: %s", asset->name) {}
            break;
          }
          case EG_PIPELINE_ASSET_TYPE: {
            ASSET_HEADER("Pipeline: %s", asset->name) {}
            break;
          }
          case EG_GLTF_MODEL_ASSET_TYPE: {
            ASSET_HEADER("GLTF model: %s", asset->name) {
              inspector_gltf_model_asset(asset);
            }
            break;
          }
          default:
            break;
          }

          igPopID();
          igSeparator();
        }

        igEndTabItem();
      }

      igEndTabBar();
    }
  }

  igEnd();
}
