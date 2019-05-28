#include "inspector.h"
#include "asset_manager.h"
#include "assets/environment_asset.h"
#include "assets/gltf_asset.h"
#include "assets/mesh_asset.h"
#include "assets/pbr_material_asset.h"
#include "comps/gltf_comp.h"
#include "comps/mesh_comp.h"
#include "comps/point_light_comp.h"
#include "comps/renderable_comp.h"
#include "comps/transform_comp.h"
#include "filesystem.h"
#include "imgui.h"
#include "pipelines.h"
#include "world.h"
#include <renderer/context.h>
#include <renderer/window.h>
#include <stb_image.h>
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

static const mat4_t gizmo_matrices[] = {
    {.cols = {{1.0, 0.0, 0.0, 0.0},
              {0.0, 1.0, 0.0, 0.0},
              {0.0, 0.0, 1.0, 0.0},
              {0.0, 0.0, 0.0, 1.0}}},
    {.cols = {{0.0, 1.0, 0.0, 0.0},
              {1.0, 0.0, 0.0, 0.0},
              {0.0, 0.0, 1.0, 0.0},
              {0.0, 0.0, 0.0, 1.0}}},
    {.cols = {{0.0, 0.0, 1.0, 0.0},
              {0.0, 1.0, 0.0, 0.0},
              {1.0, 0.0, 0.0, 0.0},
              {0.0, 0.0, 0.0, 1.0}}},
};

#define GIZMO_THICKNESS 0.05f

static const re_vertex_t pos_gizmo_vertices[] = {
    {.pos = {GIZMO_THICKNESS, GIZMO_THICKNESS, GIZMO_THICKNESS}},
    {.pos = {GIZMO_THICKNESS, -GIZMO_THICKNESS, GIZMO_THICKNESS}},
    {.pos = {GIZMO_THICKNESS, -GIZMO_THICKNESS, -GIZMO_THICKNESS}},
    {.pos = {GIZMO_THICKNESS, GIZMO_THICKNESS, -GIZMO_THICKNESS}},

    {.pos = {GIZMO_THICKNESS * 20.0f, GIZMO_THICKNESS, GIZMO_THICKNESS}},
    {.pos = {GIZMO_THICKNESS * 20.0f, -GIZMO_THICKNESS, GIZMO_THICKNESS}},
    {.pos = {GIZMO_THICKNESS * 20.0f, -GIZMO_THICKNESS, -GIZMO_THICKNESS}},
    {.pos = {GIZMO_THICKNESS * 20.0f, GIZMO_THICKNESS, -GIZMO_THICKNESS}},
};

static inline void set_selected(eg_inspector_t *inspector, eg_entity_t entity) {
  inspector->selected_entity = entity;
}

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

  eg_picker_init(
      &inspector->picker,
      window,
      inspector->drawing_render_target->width,
      inspector->drawing_render_target->height);

  eg_init_pipeline_spv(
      &inspector->gizmo_pipeline,
      inspector->drawing_render_target,
      "/shaders/gizmo.vert.spv",
      "/shaders/gizmo.frag.spv",
      eg_gizmo_pipeline_parameters());

  eg_init_pipeline_spv(
      &inspector->gizmo_picking_pipeline,
      &inspector->picker.canvas.render_target,
      "/shaders/gizmo_picking.vert.spv",
      "/shaders/gizmo_picking.frag.spv",
      eg_gizmo_pipeline_parameters());

  eg_init_pipeline_spv(
      &inspector->billboard_pipeline,
      inspector->drawing_render_target,
      "/shaders/billboard.vert.spv",
      "/shaders/billboard.frag.spv",
      eg_billboard_pipeline_parameters());

  eg_init_pipeline_spv(
      &inspector->billboard_picking_pipeline,
      &inspector->picker.canvas.render_target,
      "/shaders/billboard_picking.vert.spv",
      "/shaders/billboard_picking.frag.spv",
      eg_billboard_pipeline_parameters());

  eg_init_pipeline_spv(
      &inspector->outline_pipeline,
      inspector->drawing_render_target,
      "/shaders/outline.vert.spv",
      "/shaders/outline.frag.spv",
      eg_outline_pipeline_parameters());

  eg_init_pipeline_spv(
      &inspector->picking_pipeline,
      &inspector->picker.canvas.render_target,
      "/shaders/picking.vert.spv",
      "/shaders/picking.frag.spv",
      eg_picking_pipeline_parameters());

  {
    eg_file_t *image_file = eg_file_open_read("/assets/light.png");
    assert(image_file);
    size_t image_file_size = eg_file_size(image_file);
    unsigned char *image_file_data = calloc(1, image_file_size);
    eg_file_read_bytes(image_file, image_file_data, image_file_size);
    eg_file_close(image_file);

    int width, height, channels;
    unsigned char *image_data = stbi_load_from_memory(
        image_file_data, (int)image_file_size, &width, &height, &channels, 4);

    free(image_file_data);

    re_image_init(
        &inspector->light_billboard_image,
        &(re_image_options_t){
            .width = (uint32_t)width,
            .height = (uint32_t)height,
        });

    re_image_upload(
        &inspector->light_billboard_image,
        g_ctx.transient_command_pool,
        image_data,
        (uint32_t)width,
        (uint32_t)height,
        0,
        0);

    free(image_data);
  }

  {
    VK_CHECK(vkAllocateDescriptorSets(
        g_ctx.device,
        &(VkDescriptorSetAllocateInfo){
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = g_ctx.descriptor_pool,
            .descriptorSetCount = 1,
            .pSetLayouts = &inspector->billboard_pipeline.layout.set_layouts[1],
        },
        &inspector->light_billboard_descriptor_set));

    vkUpdateDescriptorSetWithTemplate(
        g_ctx.device,
        inspector->light_billboard_descriptor_set,
        inspector->billboard_pipeline.layout.update_templates[1],
        (re_descriptor_info_t[]){
            {.image = inspector->light_billboard_image.descriptor},
        });
  }

  inspector->drag_direction = EG_DRAG_DIRECTION_NONE;
  inspector->pos_delta = (vec3_t){0.0f, 0.0f, 0.0f};

  inspector->pos_gizmo_index_count = ARRAY_SIZE(pos_gizmo_indices);

  re_buffer_init(
      &inspector->pos_gizmo_vertex_buffer,
      &(re_buffer_options_t){.type = RE_BUFFER_TYPE_VERTEX,
                             .size = sizeof(pos_gizmo_vertices)});
  re_buffer_init(
      &inspector->pos_gizmo_index_buffer,
      &(re_buffer_options_t){.type = RE_BUFFER_TYPE_INDEX,
                             .size = sizeof(pos_gizmo_indices)});

  size_t staging_size =
      MAX(sizeof(pos_gizmo_vertices), sizeof(pos_gizmo_indices));

  re_buffer_t staging_buffer;
  re_buffer_init(
      &staging_buffer,
      &(re_buffer_options_t){.type = RE_BUFFER_TYPE_TRANSFER,
                             .size = staging_size});

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
  re_buffer_destroy(&inspector->pos_gizmo_vertex_buffer);
  re_buffer_destroy(&inspector->pos_gizmo_index_buffer);

  re_pipeline_destroy(&inspector->gizmo_pipeline);
  re_pipeline_destroy(&inspector->gizmo_picking_pipeline);
  re_pipeline_destroy(&inspector->billboard_pipeline);
  re_pipeline_destroy(&inspector->billboard_picking_pipeline);
  re_pipeline_destroy(&inspector->picking_pipeline);
  re_pipeline_destroy(&inspector->outline_pipeline);

  vkFreeDescriptorSets(
      g_ctx.device,
      g_ctx.descriptor_pool,
      1,
      &inspector->light_billboard_descriptor_set);

  re_image_destroy(&inspector->light_billboard_image);

  eg_picker_destroy(&inspector->picker);
}

static void
draw_gizmos_picking(eg_inspector_t *inspector, const eg_cmd_info_t *cmd_info) {
  re_cmd_bind_graphics_pipeline(
      cmd_info->cmd_buffer, &inspector->billboard_picking_pipeline);

  eg_camera_bind(
      &inspector->world->camera,
      cmd_info,
      &inspector->billboard_picking_pipeline,
      0);
  vkCmdBindDescriptorSets(
      cmd_info->cmd_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      inspector->billboard_picking_pipeline.layout.layout,
      1,
      1,
      &inspector->light_billboard_descriptor_set,
      0,
      NULL);

  eg_transform_comp_t *transforms =
      EG_COMP_ARRAY(inspector->world, eg_transform_comp_t);
  eg_point_light_comp_t *point_lights =
      EG_COMP_ARRAY(inspector->world, eg_point_light_comp_t);

  for (eg_entity_t e = 0; e < inspector->world->entity_max; e++) {
    if (EG_HAS_TAG(inspector->world, e, EG_TAG_HIDDEN)) {
      continue;
    }

    if (EG_HAS_COMP(inspector->world, eg_point_light_comp_t, e) &&
        EG_HAS_COMP(inspector->world, eg_transform_comp_t, e)) {
      struct {
        mat4_t model;
        uint32_t index;
      } push_constant;

      push_constant.model = eg_transform_comp_mat4(&transforms[e]);
      push_constant.index = e;

      vkCmdPushConstants(
          cmd_info->cmd_buffer,
          inspector->billboard_picking_pipeline.layout.layout,
          inspector->billboard_picking_pipeline.layout.push_constants[0]
              .stageFlags,
          inspector->billboard_picking_pipeline.layout.push_constants[0].offset,
          sizeof(push_constant),
          &push_constant);

      vkCmdDraw(cmd_info->cmd_buffer, 6, 1, 0, 0);
    }
  }

  if (inspector->selected_entity >= EG_MAX_ENTITIES) {
    return;
  }

  if (!EG_HAS_COMP(
          inspector->world, eg_transform_comp_t, inspector->selected_entity)) {
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
          .rect.extent.width = inspector->picker.canvas.render_target.width,
          .rect.extent.height = inspector->picker.canvas.render_target.height,
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

  eg_transform_comp_t *transform = EG_COMP(
      inspector->world, eg_transform_comp_t, inspector->selected_entity);

  mat4_t object_mat = eg_transform_comp_mat4(transform);

  uint32_t color_indices[] = {
      EG_DRAG_DIRECTION_X, EG_DRAG_DIRECTION_Y, EG_DRAG_DIRECTION_Z};

  for (uint32_t i = 0; i < 3; i++) {
    mat4_t mat = gizmo_matrices[i];
    mat.cols[3][0] = object_mat.cols[3][0];
    mat.cols[3][1] = object_mat.cols[3][1];
    mat.cols[3][2] = object_mat.cols[3][2];

    push_constant.mvp = mat4_mul(
        mat,
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

  const eg_cmd_info_t cmd_info = eg_picker_begin(&inspector->picker);

  re_cmd_bind_graphics_pipeline(
      cmd_info.cmd_buffer, &inspector->picking_pipeline);

  eg_camera_bind(
      &inspector->world->camera, &cmd_info, &inspector->picking_pipeline, 0);

  eg_transform_comp_t *transforms =
      EG_COMP_ARRAY(inspector->world, eg_transform_comp_t);
  eg_gltf_comp_t *gltf_models = EG_COMP_ARRAY(inspector->world, eg_gltf_comp_t);
  eg_mesh_comp_t *meshes = EG_COMP_ARRAY(inspector->world, eg_mesh_comp_t);

#define PUSH_CONSTANT()                                                        \
  vkCmdPushConstants(                                                          \
      cmd_info.cmd_buffer,                                                     \
      inspector->picking_pipeline.layout.layout,                               \
      inspector->picking_pipeline.layout.push_constants[0].stageFlags,         \
      inspector->picking_pipeline.layout.push_constants[0].offset,             \
      sizeof(uint32_t),                                                        \
      &e)

  for (eg_entity_t e = 0; e < inspector->world->entity_max; e++) {
    if (EG_HAS_COMP(inspector->world, eg_mesh_comp_t, e) &&
        EG_HAS_COMP(inspector->world, eg_transform_comp_t, e)) {
      PUSH_CONSTANT();

      meshes[e].model.uniform.transform =
          eg_transform_comp_mat4(&transforms[e]);
      eg_mesh_comp_draw_no_mat(
          &meshes[e], &cmd_info, &inspector->picking_pipeline);
    }

    if (EG_HAS_COMP(inspector->world, eg_gltf_comp_t, e) &&
        EG_HAS_COMP(inspector->world, eg_transform_comp_t, e)) {
      PUSH_CONSTANT();

      eg_gltf_comp_draw_no_mat(
          &gltf_models[e],
          &cmd_info,
          &inspector->picking_pipeline,
          eg_transform_comp_mat4(&transforms[e]));
    }
  }

  draw_gizmos_picking(inspector, &cmd_info);

  eg_picker_end(&inspector->picker);

  double cursor_x, cursor_y;
  re_window_get_cursor_pos(inspector->window, &cursor_x, &cursor_y);

  uint32_t selected =
      eg_picker_get(&inspector->picker, (uint32_t)cursor_x, (uint32_t)cursor_y);

  switch (selected) {
  case EG_DRAG_DIRECTION_X:
  case EG_DRAG_DIRECTION_Y:
  case EG_DRAG_DIRECTION_Z: {
    inspector->drag_direction = selected;
    break;
  }
  default: {
    set_selected(inspector, selected);
    break;
  }
  }

  if (inspector->selected_entity < EG_MAX_ENTITIES &&
      EG_HAS_COMP(
          inspector->world, eg_transform_comp_t, inspector->selected_entity)) {
    eg_transform_comp_t *transform = EG_COMP(
        inspector->world, eg_transform_comp_t, inspector->selected_entity);
    vec3_t transform_ndc =
        eg_camera_world_to_ndc(&inspector->world->camera, transform->position);

    float width = (float)inspector->drawing_render_target->width;
    float height = (float)inspector->drawing_render_target->height;

    float nx = (((float)cursor_x / width) * 2.0f) - 1.0f;
    float ny = (((float)cursor_y / height) * 2.0f) - 1.0f;
    vec3_t cursor_ndc = {nx, ny, transform_ndc.z};
    vec3_t cursor_world =
        eg_camera_ndc_to_world(&inspector->world->camera, cursor_ndc);

    inspector->pos_delta = vec3_sub(transform->position, cursor_world);
  }
}

void eg_inspector_process_event(
    eg_inspector_t *inspector, const re_event_t *event) {
  switch (event->type) {
  case RE_EVENT_FRAMEBUFFER_RESIZED: {
    eg_picker_resize(
        &inspector->picker,
        (uint32_t)event->size.width,
        (uint32_t)event->size.height);
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
      inspector->pos_delta = (vec3_t){0.0f, 0.0f, 0.0f};
      inspector->drag_direction = EG_DRAG_DIRECTION_NONE;
    }
    break;
  }
  default: break;
  }
}

void eg_inspector_update(eg_inspector_t *inspector) {
  if (inspector->drag_direction == EG_DRAG_DIRECTION_NONE ||
      inspector->selected_entity >= EG_MAX_ENTITIES ||
      !EG_HAS_COMP(
          inspector->world, eg_transform_comp_t, inspector->selected_entity)) {
    return;
  }

  eg_transform_comp_t *transform = EG_COMP(
      inspector->world, eg_transform_comp_t, inspector->selected_entity);
  vec3_t transform_ndc =
      eg_camera_world_to_ndc(&inspector->world->camera, transform->position);

  double cursor_x, cursor_y;
  re_window_get_cursor_pos(inspector->window, &cursor_x, &cursor_y);

  float width = (float)inspector->drawing_render_target->width;
  float height = (float)inspector->drawing_render_target->height;

  float nx = (((float)cursor_x / width) * 2.0f) - 1.0f;
  float ny = (((float)cursor_y / height) * 2.0f) - 1.0f;
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

static inline int light_cmp(const void *e1, const void *e2, void *context) {
  return 0;
}

void eg_inspector_draw_gizmos(
    eg_inspector_t *inspector, const eg_cmd_info_t *cmd_info) {

  re_cmd_bind_graphics_pipeline(
      cmd_info->cmd_buffer, &inspector->billboard_pipeline);

  eg_camera_bind(
      &inspector->world->camera, cmd_info, &inspector->billboard_pipeline, 0);
  vkCmdBindDescriptorSets(
      cmd_info->cmd_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      inspector->billboard_pipeline.layout.layout,
      1,
      1,
      &inspector->light_billboard_descriptor_set,
      0,
      NULL);

  eg_transform_comp_t *transforms =
      EG_COMP_ARRAY(inspector->world, eg_transform_comp_t);
  eg_point_light_comp_t *point_lights =
      EG_COMP_ARRAY(inspector->world, eg_point_light_comp_t);

  static eg_entity_t light_entities[EG_MAX_POINT_LIGHTS];
  uint32_t light_count = 0;

  for (eg_entity_t e = 0; e < inspector->world->entity_max; e++) {
    if (EG_HAS_TAG(inspector->world, e, EG_TAG_HIDDEN)) {
      continue;
    }

    if (EG_HAS_COMP(inspector->world, eg_point_light_comp_t, e) &&
        EG_HAS_COMP(inspector->world, eg_transform_comp_t, e)) {
      light_entities[light_count++] = e;
    }
  }

  // Draw call sorting
  for (uint32_t i = 0; i < light_count; i++) {
    for (uint32_t j = 0; j < light_count - i; j++) {
      if (vec3_distance(
              transforms[light_entities[i]].position,
              inspector->world->camera.uniform.pos.xyz) >
          vec3_distance(
              transforms[light_entities[j]].position,
              inspector->world->camera.uniform.pos.xyz)) {
        eg_entity_t t = light_entities[i];
        light_entities[i] = light_entities[j];
        light_entities[j] = t;
      }
    }
  }

  for (uint32_t i = 0; i < light_count; i++) {
    struct {
      mat4_t model;
      vec4_t color;
    } push_constant;

    push_constant.model =
        eg_transform_comp_mat4(&transforms[light_entities[i]]);
    push_constant.color = point_lights[light_entities[i]].color;

    vkCmdPushConstants(
        cmd_info->cmd_buffer,
        inspector->billboard_pipeline.layout.layout,
        inspector->billboard_pipeline.layout.push_constants[0].stageFlags,
        inspector->billboard_pipeline.layout.push_constants[0].offset,
        sizeof(push_constant),
        &push_constant);

    vkCmdDraw(cmd_info->cmd_buffer, 6, 1, 0, 0);
  }

  if (inspector->selected_entity >= EG_MAX_ENTITIES) {
    return;
  }

  if (!EG_HAS_COMP(
          inspector->world, eg_transform_comp_t, inspector->selected_entity)) {
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

  eg_transform_comp_t *transform = EG_COMP(
      inspector->world, eg_transform_comp_t, inspector->selected_entity);

  mat4_t object_mat = eg_transform_comp_mat4(transform);

  vec4_t colors[] = {{1.0f, 0.0f, 0.0f, 0.5f},
                     {0.0f, 1.0f, 0.0f, 0.5f},
                     {0.0f, 0.0f, 1.0f, 0.5f}};

  for (uint32_t i = 0; i < 3; i++) {
    mat4_t mat = gizmo_matrices[i];
    mat.cols[3][0] = object_mat.cols[3][0];
    mat.cols[3][1] = object_mat.cols[3][1];
    mat.cols[3][2] = object_mat.cols[3][2];

    push_constant.mvp = mat4_mul(
        mat,
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

void eg_inspector_draw_selected_outline(
    eg_inspector_t *inspector, const eg_cmd_info_t *cmd_info) {
  re_cmd_bind_graphics_pipeline(
      cmd_info->cmd_buffer, &inspector->outline_pipeline);

  eg_camera_bind(
      &inspector->world->camera, cmd_info, &inspector->outline_pipeline, 0);

  vec4_t color = {1.0f, 0.5f, 0.0f, 1.0f};

  vkCmdPushConstants(
      cmd_info->cmd_buffer,
      inspector->outline_pipeline.layout.layout,
      inspector->outline_pipeline.layout.push_constants[0].stageFlags,
      inspector->outline_pipeline.layout.push_constants[0].offset,
      sizeof(color),
      &color);

  if (inspector->selected_entity < EG_MAX_ENTITIES &&
      EG_HAS_COMP(
          inspector->world, eg_gltf_comp_t, inspector->selected_entity) &&
      EG_HAS_COMP(
          inspector->world, eg_transform_comp_t, inspector->selected_entity)) {
    eg_gltf_comp_t *model =
        EG_COMP(inspector->world, eg_gltf_comp_t, inspector->selected_entity);
    eg_transform_comp_t *transform = EG_COMP(
        inspector->world, eg_transform_comp_t, inspector->selected_entity);

    eg_gltf_comp_draw_no_mat(
        model,
        cmd_info,
        &inspector->outline_pipeline,
        eg_transform_comp_mat4(transform));
  }

  if (inspector->selected_entity < EG_MAX_ENTITIES &&
      EG_HAS_COMP(
          inspector->world, eg_mesh_comp_t, inspector->selected_entity) &&
      EG_HAS_COMP(
          inspector->world, eg_transform_comp_t, inspector->selected_entity)) {
    eg_mesh_comp_t *mesh =
        EG_COMP(inspector->world, eg_mesh_comp_t, inspector->selected_entity);
    eg_transform_comp_t *transform = EG_COMP(
        inspector->world, eg_transform_comp_t, inspector->selected_entity);

    mesh->model.uniform.transform = eg_transform_comp_mat4(transform);
    eg_mesh_comp_draw_no_mat(mesh, cmd_info, &inspector->outline_pipeline);
  }
}

static void inspect_camera(eg_camera_t *camera) {
  float deg = to_degrees(camera->fov);
  igDragFloat("FOV", &deg, 0.1f, 0.0f, 0.0f, "%.3f", 1.0f);
  camera->fov = to_radians(deg);

  igDragFloat3(
      "Position", &camera->position.x, 0.01f, 0.0f, 0.0f, "%.3f", 1.0f);

  igDragFloat4(
      "Rotation", &camera->rotation.x, 0.01f, 0.0f, 0.0f, "%.3f", 1.0f);
}

static void inspect_environment(eg_environment_t *environment) {
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

  static int current_item = 0;
  const char *const items[] = {"Default", "Irradiance"};

  if (igListBoxStr_arr(
          "Skybox type",
          &current_item,
          items,
          ARRAY_SIZE(items),
          ARRAY_SIZE(items))) {
    environment->skybox_type = (eg_skybox_type_t)current_item;
  }
}

static void inspect_statistics(re_window_t *window) {
  igText("Delta time: %.4fms", window->delta_time);
  igText("FPS: %.2f", 1.0f / window->delta_time);

  igText(
      "Descriptor set allocator count: %u",
      g_ctx.descriptor_set_allocator_count);
  for (uint32_t i = 0; i < g_ctx.descriptor_set_allocator_count; i++) {
    uint32_t node_count = 0;
    re_descriptor_set_allocator_node_t *node =
        &g_ctx.descriptor_set_allocators[i].base_node;
    while (node != NULL) {
      node_count++;
      node = node->next;
    }
    igText("Allocator #%d: %u nodes", i, node_count);
  }
}

static void inspect_environment_asset(eg_asset_t *asset) {}

static void inspect_pbr_material_asset(eg_asset_t *asset) {
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

static void inspect_gltf_model_asset(eg_asset_t *asset) {
  eg_gltf_asset_t *gltf_asset = (eg_gltf_asset_t *)asset;

  igText("Vertex count: %u", gltf_asset->vertex_count);
  igText("Index count: %u", gltf_asset->index_count);
  igText("Image count: %u", gltf_asset->image_count);
  igText("Mesh count: %u", gltf_asset->mesh_count);

  for (uint32_t j = 0; j < gltf_asset->material_count; j++) {
    igPushIDInt(j);
    eg_gltf_asset_material_t *material = &gltf_asset->materials[j];

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

static void inspect_transform_comp(eg_world_t *world, eg_entity_t entity) {
  eg_transform_comp_t *transform = EG_COMP(world, eg_transform_comp_t, entity);

  igDragFloat3(
      "Position", &transform->position.x, 0.1f, 0.0f, 0.0f, "%.3f", 1.0f);
  igDragFloat3("Scale", &transform->scale.x, 0.1f, 0.0f, 0.0f, "%.3f", 1.0f);
  igDragFloat3("Axis", &transform->axis.x, 0.01f, 0.0f, 1.0f, "%.3f", 1.0f);
  igDragFloat("Angle", &transform->angle, 0.01f, 0.0f, 0.0f, "%.3f rad", 1.0f);
}

static void inspect_point_light_comp(eg_world_t *world, eg_entity_t entity) {
  eg_point_light_comp_t *point_light =
      EG_COMP(world, eg_point_light_comp_t, entity);
  igColorEdit4("Color", &point_light->color.r, 0);
  igDragFloat(
      "Intensity", &point_light->intensity, 0.01f, 0.0f, 0.0f, "%.3f", 1.0f);
}

static void inspect_renderable_comp(eg_world_t *world, eg_entity_t entity) {
  eg_renderable_comp_t *renderable =
      EG_COMP(world, eg_renderable_comp_t, entity);

  igText("Pipeline asset: %s", renderable->pipeline->asset.name);
}

static void inspect_mesh_comp(eg_world_t *world, eg_entity_t entity) {
  eg_mesh_comp_t *mesh = EG_COMP(world, eg_mesh_comp_t, entity);

  igText("Material: %s", mesh->material->asset.name);
  igSameLine(0.0f, -1.0f);
  if (igSmallButton("Inspect")) {
    igOpenPopup("meshmaterialpopup");
  }

  if (igBeginPopup("meshmaterialpopup", 0)) {
    inspect_pbr_material_asset((eg_asset_t *)mesh->material);
    igEndPopup();
  }
}

static void inspect_gltf_model_comp(eg_world_t *world, eg_entity_t entity) {
  eg_gltf_comp_t *gltf_model = EG_COMP(world, eg_gltf_comp_t, entity);

  igText("Asset: %s", gltf_model->asset->asset.name);
  igSameLine(0.0f, -1.0f);
  if (igSmallButton("Inspect")) {
    igOpenPopup("gltfmodelpopup");
  }

  if (igBeginPopup("gltfmodelpopup", 0)) {
    inspect_gltf_model_asset((eg_asset_t *)gltf_model->asset);
    igEndPopup();
  }
}

static inline void selected_entity_ui(eg_inspector_t *inspector) {
  eg_world_t *world = inspector->world;

  if (eg_world_exists(world, inspector->selected_entity)) {
    if (igBegin("Selected entity", NULL, 0)) {
      eg_entity_t entity = inspector->selected_entity;

      igText("Entity #%u", inspector->selected_entity);
      igSameLine(0.0f, -1.0f);
      if (igSmallButton("Remove")) {
        eg_world_remove(world, inspector->selected_entity);
        set_selected(inspector, UINT32_MAX);
        igEnd();
        return;
      }

      if (igCollapsingHeader("Tags", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (eg_tag_t tag = 0; tag < EG_TAG_MAX; tag++) {
          bool has_tag =
              EG_HAS_TAG(inspector->world, inspector->selected_entity, tag);
          igCheckbox(EG_TAG_NAMES[tag], &has_tag);
          EG_SET_TAG(
              inspector->world, inspector->selected_entity, tag, has_tag);
        }
      }

      if (EG_HAS_COMP(world, eg_transform_comp_t, entity) &&
          igCollapsingHeader(
              EG_COMP_NAME(eg_transform_comp_t),
              ImGuiTreeNodeFlags_DefaultOpen)) {
        inspect_transform_comp(world, entity);
      }

      if (EG_HAS_COMP(world, eg_point_light_comp_t, entity) &&
          igCollapsingHeader(
              EG_COMP_NAME(eg_point_light_comp_t),
              ImGuiTreeNodeFlags_DefaultOpen)) {
        inspect_point_light_comp(world, entity);
      }

      if (EG_HAS_COMP(world, eg_renderable_comp_t, entity) &&
          igCollapsingHeader(
              EG_COMP_NAME(eg_renderable_comp_t),
              ImGuiTreeNodeFlags_DefaultOpen)) {
        inspect_renderable_comp(world, entity);
      }

      if (EG_HAS_COMP(world, eg_mesh_comp_t, entity) &&
          igCollapsingHeader(
              EG_COMP_NAME(eg_mesh_comp_t), ImGuiTreeNodeFlags_DefaultOpen)) {
        inspect_mesh_comp(world, entity);
      }

      if (EG_HAS_COMP(world, eg_gltf_comp_t, entity) &&
          igCollapsingHeader(
              EG_COMP_NAME(eg_gltf_comp_t), ImGuiTreeNodeFlags_DefaultOpen)) {
        inspect_gltf_model_comp(world, entity);
      }
    }
    igEnd();
  }
}

void eg_inspector_draw_ui(eg_inspector_t *inspector) {
  static char str[256] = "";

  re_window_t *window = inspector->window;
  eg_world_t *world = inspector->world;
  eg_asset_manager_t *asset_manager = inspector->asset_manager;

  selected_entity_ui(inspector);

  if (igBegin("Inspector", NULL, 0)) {
    if (igBeginTabBar("Inspector", 0)) {
      if (igBeginTabItem("World", NULL, 0)) {
        if (igCollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
          inspect_camera(&world->camera);
        }

        if (igCollapsingHeader("Environment", ImGuiTreeNodeFlags_DefaultOpen)) {
          inspect_environment(&world->environment);
        }

        igEndTabItem();
      }

      if (igBeginTabItem("Statistics", NULL, 0)) {
        inspect_statistics(window);
        igEndTabItem();
      }

      if (igBeginTabItem("Entities", NULL, 0)) {
        if (igSmallButton("Add entity")) {
          eg_world_add(world);
        }

        for (eg_entity_t entity = 0; entity < world->entity_max; entity++) {
          if (!eg_world_exists(world, entity)) {
            continue;
          }

          snprintf(str, sizeof(str), "Entity #%d", entity);
          if (igSelectable(
                  str,
                  inspector->selected_entity == entity,
                  0,
                  (ImVec2){0.0f, 0.0f})) {
            set_selected(inspector, entity);
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
              inspect_environment_asset(asset);
            }
            break;
          }
          case EG_PBR_MATERIAL_ASSET_TYPE: {
            ASSET_HEADER("Material: %s", asset->name) {
              inspect_pbr_material_asset(asset);
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
          case EG_GLTF_ASSET_TYPE: {
            ASSET_HEADER("GLTF model: %s", asset->name) {
              inspect_gltf_model_asset(asset);
            }
            break;
          }
          default: break;
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
