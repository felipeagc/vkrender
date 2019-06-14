#include "inspector.h"

#include "asset_manager.h"
#include "comps/gltf_comp.h"
#include "comps/mesh_comp.h"
#include "comps/point_light_comp.h"
#include "comps/transform_comp.h"
#include "filesystem.h"
#include "imgui.h"
#include "pipelines.h"
#include "world.h"
#include <float.h>
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
  memset(inspector, 0, sizeof(*inspector));

  inspector->selected_entity       = UINT32_MAX;
  inspector->window                = window;
  inspector->world                 = world;
  inspector->asset_manager         = asset_manager;
  inspector->drawing_render_target = render_target;
  inspector->snapping              = 0.1f;
  inspector->snap                  = false;

  eg_picker_init(
      &inspector->picker,
      window,
      inspector->drawing_render_target->width,
      inspector->drawing_render_target->height);

  eg_init_pipeline_spv(
      &inspector->gizmo_pipeline,
      inspector->drawing_render_target,
      (const char *[]){"/shaders/gizmo.vert.spv", "/shaders/gizmo.frag.spv"},
      2,
      eg_gizmo_pipeline_parameters());

  eg_init_pipeline_spv(
      &inspector->gizmo_picking_pipeline,
      &inspector->picker.canvas.render_target,
      (const char *[]){"/shaders/gizmo_picking.vert.spv",
                       "/shaders/gizmo_picking.frag.spv"},
      2,
      eg_gizmo_pipeline_parameters());

  eg_init_pipeline_spv(
      &inspector->billboard_pipeline,
      inspector->drawing_render_target,
      (const char *[]){"/shaders/billboard.vert.spv",
                       "/shaders/billboard.frag.spv"},
      2,
      eg_billboard_pipeline_parameters());

  eg_init_pipeline_spv(
      &inspector->billboard_picking_pipeline,
      &inspector->picker.canvas.render_target,
      (const char *[]){"/shaders/billboard_picking.vert.spv",
                       "/shaders/billboard_picking.frag.spv"},
      2,
      eg_billboard_pipeline_parameters());

  eg_init_pipeline_spv(
      &inspector->outline_pipeline,
      inspector->drawing_render_target,
      (const char *[]){"/shaders/outline.vert.spv",
                       "/shaders/outline.frag.spv"},
      2,
      eg_outline_pipeline_parameters());

  eg_init_pipeline_spv(
      &inspector->picking_pipeline,
      &inspector->picker.canvas.render_target,
      (const char *[]){"/shaders/picking.vert.spv",
                       "/shaders/picking.frag.spv"},
      2,
      eg_picking_pipeline_parameters());

  {
    eg_file_t *image_file = eg_file_open_read("/assets/light.png");
    assert(image_file);
    size_t image_file_size         = eg_file_size(image_file);
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
            .width  = (uint32_t)width,
            .height = (uint32_t)height,
            .usage  = RE_IMAGE_USAGE_SAMPLED | RE_IMAGE_USAGE_TRANSFER_DST,
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

  inspector->drag_direction = EG_DRAG_DIRECTION_NONE;
  inspector->pos_delta      = (vec3_t){0.0f, 0.0f, 0.0f};

  inspector->pos_gizmo_index_count = ARRAY_SIZE(pos_gizmo_indices);

  re_buffer_init(
      &inspector->pos_gizmo_vertex_buffer,
      &(re_buffer_options_t){.usage  = RE_BUFFER_USAGE_VERTEX,
                             .memory = RE_BUFFER_MEMORY_DEVICE,
                             .size   = sizeof(pos_gizmo_vertices)});
  re_buffer_init(
      &inspector->pos_gizmo_index_buffer,
      &(re_buffer_options_t){.usage  = RE_BUFFER_USAGE_INDEX,
                             .memory = RE_BUFFER_MEMORY_DEVICE,
                             .size   = sizeof(pos_gizmo_indices)});

  size_t staging_size =
      MAX(sizeof(pos_gizmo_vertices), sizeof(pos_gizmo_indices));

  re_buffer_t staging_buffer;
  re_buffer_init(
      &staging_buffer,
      &(re_buffer_options_t){.usage  = RE_BUFFER_USAGE_TRANSFER,
                             .memory = RE_BUFFER_MEMORY_HOST,
                             .size   = staging_size});

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

  re_image_destroy(&inspector->light_billboard_image);

  eg_picker_destroy(&inspector->picker);
}

static void
draw_gizmos_picking(eg_inspector_t *inspector, re_cmd_buffer_t *cmd_buffer) {
  re_cmd_bind_pipeline(cmd_buffer, &inspector->billboard_picking_pipeline);

  eg_camera_bind(
      &inspector->world->camera,
      cmd_buffer,
      &inspector->billboard_picking_pipeline,
      0);

  re_cmd_bind_image(cmd_buffer, 1, 0, &inspector->light_billboard_image);
  re_cmd_bind_descriptor_set(cmd_buffer, &inspector->billboard_pipeline, 1);

  eg_transform_comp_t *transforms =
      EG_COMP_ARRAY(inspector->world, eg_transform_comp_t);

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

      re_cmd_push_constants(
          cmd_buffer,
          &inspector->billboard_picking_pipeline,
          0,
          sizeof(push_constant),
          &push_constant);

      re_cmd_draw(cmd_buffer, 6, 1, 0, 0);
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
      cmd_buffer->cmd_buffer,
      1,
      &(VkClearAttachment){
          .aspectMask                      = VK_IMAGE_ASPECT_DEPTH_BIT,
          .clearValue.depthStencil.depth   = 1.0f,
          .clearValue.depthStencil.stencil = 0,
      },
      1,
      &(VkClearRect){
          .rect.offset.x      = 0,
          .rect.offset.y      = 0,
          .rect.extent.width  = inspector->picker.canvas.render_target.width,
          .rect.extent.height = inspector->picker.canvas.render_target.height,
          .baseArrayLayer     = 0,
          .layerCount         = 1,
      });

  re_cmd_bind_pipeline(cmd_buffer, &inspector->gizmo_picking_pipeline);

  re_cmd_bind_index_buffer(
      cmd_buffer, &inspector->pos_gizmo_index_buffer, 0, RE_INDEX_TYPE_UINT32);

  size_t offsets = 0;
  re_cmd_bind_vertex_buffers(
      cmd_buffer, 0, 1, &inspector->pos_gizmo_vertex_buffer, &offsets);

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
    mat4_t mat     = gizmo_matrices[i];
    mat.cols[3][0] = object_mat.cols[3][0];
    mat.cols[3][1] = object_mat.cols[3][1];
    mat.cols[3][2] = object_mat.cols[3][2];

    push_constant.mvp = mat4_mul(
        mat,
        mat4_mul(
            inspector->world->camera.uniform.view,
            inspector->world->camera.uniform.proj));
    push_constant.index = color_indices[i];

    re_cmd_push_constants(
        cmd_buffer,
        &inspector->gizmo_picking_pipeline,
        0,
        sizeof(push_constant),
        &push_constant);

    re_cmd_draw_indexed(
        cmd_buffer, inspector->pos_gizmo_index_count, 1, 0, 0, 0);
  }
}

static void mouse_pressed(eg_inspector_t *inspector) {
  if (igIsWindowHovered(
          ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_ChildWindows |
          ImGuiHoveredFlags_AllowWhenBlockedByPopup |
          ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
    return;
  }

  re_cmd_buffer_t *cmd_buffer = eg_picker_begin(&inspector->picker);

  re_cmd_bind_pipeline(cmd_buffer, &inspector->picking_pipeline);

  eg_camera_bind(
      &inspector->world->camera, cmd_buffer, &inspector->picking_pipeline, 0);

  eg_transform_comp_t *transforms =
      EG_COMP_ARRAY(inspector->world, eg_transform_comp_t);
  eg_gltf_comp_t *gltf_models = EG_COMP_ARRAY(inspector->world, eg_gltf_comp_t);
  eg_mesh_comp_t *meshes      = EG_COMP_ARRAY(inspector->world, eg_mesh_comp_t);

#define PUSH_CONSTANT()                                                        \
  re_cmd_push_constants(                                                       \
      cmd_buffer, &inspector->picking_pipeline, 0, sizeof(uint32_t), &e);

  for (eg_entity_t e = 0; e < inspector->world->entity_max; e++) {
    if (EG_HAS_COMP(inspector->world, eg_mesh_comp_t, e) &&
        EG_HAS_COMP(inspector->world, eg_transform_comp_t, e)) {
      PUSH_CONSTANT();

      eg_mesh_comp_draw_no_mat(
          &meshes[e],
          cmd_buffer,
          &inspector->picking_pipeline,
          eg_transform_comp_mat4(&transforms[e]));
    }

    if (EG_HAS_COMP(inspector->world, eg_gltf_comp_t, e) &&
        EG_HAS_COMP(inspector->world, eg_transform_comp_t, e)) {
      PUSH_CONSTANT();

      eg_gltf_comp_draw_no_mat(
          &gltf_models[e],
          cmd_buffer,
          &inspector->picking_pipeline,
          eg_transform_comp_mat4(&transforms[e]));
    }
  }

  draw_gizmos_picking(inspector, cmd_buffer);

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

    float width  = (float)inspector->drawing_render_target->width;
    float height = (float)inspector->drawing_render_target->height;

    float nx          = (((float)cursor_x / width) * 2.0f) - 1.0f;
    float ny          = (((float)cursor_y / height) * 2.0f) - 1.0f;
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
      inspector->pos_delta      = (vec3_t){0.0f, 0.0f, 0.0f};
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

  float width  = (float)inspector->drawing_render_target->width;
  float height = (float)inspector->drawing_render_target->height;

  float nx          = (((float)cursor_x / width) * 2.0f) - 1.0f;
  float ny          = (((float)cursor_y / height) * 2.0f) - 1.0f;
  vec3_t cursor_ndc = {nx, ny, transform_ndc.z};
  vec3_t cursor_world =
      eg_camera_ndc_to_world(&inspector->world->camera, cursor_ndc);

#define SNAP(x) (inspector->snap ? (x - fmodf((x), inspector->snapping)) : (x))

  switch (inspector->drag_direction) {
  case EG_DRAG_DIRECTION_X: {
    transform->position.x = SNAP(cursor_world.x) + SNAP(inspector->pos_delta.x);
    break;
  }
  case EG_DRAG_DIRECTION_Y: {
    transform->position.y = SNAP(cursor_world.y) + SNAP(inspector->pos_delta.y);
    break;
  }
  case EG_DRAG_DIRECTION_Z: {
    transform->position.z = SNAP(cursor_world.z) + SNAP(inspector->pos_delta.z);
    break;
  }
  default: {
    break;
  }
  }
}

void eg_inspector_draw_gizmos(
    eg_inspector_t *inspector, re_cmd_buffer_t *cmd_buffer) {

  re_cmd_bind_pipeline(cmd_buffer, &inspector->billboard_pipeline);

  eg_camera_bind(
      &inspector->world->camera, cmd_buffer, &inspector->billboard_pipeline, 0);

  re_cmd_bind_image(cmd_buffer, 1, 0, &inspector->light_billboard_image);
  re_cmd_bind_descriptor_set(cmd_buffer, &inspector->billboard_pipeline, 1);

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
        eg_entity_t t     = light_entities[i];
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

    re_cmd_push_constants(
        cmd_buffer,
        &inspector->billboard_pipeline,
        0,
        sizeof(push_constant),
        &push_constant);

    re_cmd_draw(cmd_buffer, 6, 1, 0, 0);
  }

  if (inspector->selected_entity >= EG_MAX_ENTITIES) {
    return;
  }

  if (!EG_HAS_COMP(
          inspector->world, eg_transform_comp_t, inspector->selected_entity)) {
    return;
  }

  vkCmdClearAttachments(
      cmd_buffer->cmd_buffer,
      1,
      &(VkClearAttachment){
          .aspectMask                      = VK_IMAGE_ASPECT_DEPTH_BIT,
          .clearValue.depthStencil.depth   = 1.0f,
          .clearValue.depthStencil.stencil = 0,
      },
      1,
      &(VkClearRect){
          .rect.offset.x      = 0,
          .rect.offset.y      = 0,
          .rect.extent.width  = inspector->drawing_render_target->width,
          .rect.extent.height = inspector->drawing_render_target->height,
          .baseArrayLayer     = 0,
          .layerCount         = 1,
      });

  re_cmd_bind_pipeline(cmd_buffer, &inspector->gizmo_pipeline);

  re_cmd_bind_index_buffer(
      cmd_buffer, &inspector->pos_gizmo_index_buffer, 0, RE_INDEX_TYPE_UINT32);

  size_t offsets = 0;
  re_cmd_bind_vertex_buffers(
      cmd_buffer, 0, 1, &inspector->pos_gizmo_vertex_buffer, &offsets);

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
    mat4_t mat     = gizmo_matrices[i];
    mat.cols[3][0] = object_mat.cols[3][0];
    mat.cols[3][1] = object_mat.cols[3][1];
    mat.cols[3][2] = object_mat.cols[3][2];

    push_constant.mvp = mat4_mul(
        mat,
        mat4_mul(
            inspector->world->camera.uniform.view,
            inspector->world->camera.uniform.proj));
    push_constant.color = colors[i];

    re_cmd_push_constants(
        cmd_buffer,
        &inspector->gizmo_pipeline,
        0,
        sizeof(push_constant),
        &push_constant);

    re_cmd_draw_indexed(
        cmd_buffer, inspector->pos_gizmo_index_count, 1, 0, 0, 0);
  }
}

void eg_inspector_draw_selected_outline(
    eg_inspector_t *inspector, re_cmd_buffer_t *cmd_buffer) {
  re_cmd_bind_pipeline(cmd_buffer, &inspector->outline_pipeline);

  eg_camera_bind(
      &inspector->world->camera, cmd_buffer, &inspector->outline_pipeline, 0);

  vec4_t color = {1.0f, 0.5f, 0.0f, 1.0f};

  re_cmd_push_constants(
      cmd_buffer, &inspector->outline_pipeline, 0, sizeof(color), &color);

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
        cmd_buffer,
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

    eg_mesh_comp_draw_no_mat(
        mesh,
        cmd_buffer,
        &inspector->outline_pipeline,
        eg_transform_comp_mat4(transform));
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

  static int current_item   = 0;
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
  igText("");

#define FRAME_TIMES_MAX 512

  static float frame_times[FRAME_TIMES_MAX] = {0};
  static int frame                          = 0;

  frame_times[frame] = (float)window->delta_time;
  frame              = (frame + 1) % FRAME_TIMES_MAX;

  igPlotLines(
      "Frame time",
      frame_times,
      FRAME_TIMES_MAX,
      frame,
      NULL,
      0.0f,
      1.0f / 60.0f,
      (ImVec2){0, 100},
      sizeof(float));

  igText(
      "Descriptor set allocator count: %u",
      g_ctx.descriptor_set_allocator_count);
  for (uint32_t i = 0; i < g_ctx.descriptor_set_allocator_count; i++) {
    re_descriptor_set_allocator_t *allocator =
        &g_ctx.descriptor_set_allocators[i];

    uint32_t node_count = 0;
    re_descriptor_set_allocator_node_t *node =
        &allocator->base_nodes[allocator->current_frame];
    while (node != NULL) {
      node_count++;
      node = node->next;
    }

    igText(
        "Allocator #%d:\tnodes: %u\twrite rate: %u/%u\niters: %u",
        i,
        node_count,
        allocator->writes[allocator->current_frame],
        allocator->writes[allocator->current_frame] +
            allocator->matches[allocator->current_frame],
        allocator->max_iterations[allocator->current_frame]);
  }

  igText("");

  {
    uint32_t node_count = 0;
    re_buffer_pool_node_t *node =
        &g_ctx.ubo_pool.base_nodes[g_ctx.ubo_pool.current_frame];
    while (node != NULL) {
      node_count++;
      node = node->next;
    }
    igText("UBO pool: %u nodes", node_count);
  }
}

static void inspect_settings(eg_inspector_t *inspector) {
  igCheckbox("Snapping", &inspector->snap);
  igDragFloat("Snap to", &inspector->snapping, 0.01f, 0.0f, 0.0f, "%.3f", 1.0f);
}

void add_component_button(eg_inspector_t *inspector, eg_entity_t entity) {
  if (igButton(
          "Add component", (ImVec2){igGetContentRegionAvailWidth(), 30.0f})) {
    igOpenPopup("addcomp");
  }

  if (igBeginPopup("addcomp", 0)) {
    for (uint32_t comp_id = 0; comp_id < EG_COMP_TYPE_MAX; comp_id++) {
      if (EG_HAS_COMP_ID(inspector->world, comp_id, entity)) {
        continue;
      }
      if (igSelectable(
              EG_COMP_NAMES[comp_id], false, 0, (ImVec2){0.0f, 0.0f})) {
        eg_world_add_comp(inspector->world, comp_id, entity);
      }
    }

    igEndPopup();
  }
}

void eg_inspector_draw_ui(eg_inspector_t *inspector) {
  static char str[256] = "";

  re_window_t *window               = inspector->window;
  eg_world_t *world                 = inspector->world;
  eg_asset_manager_t *asset_manager = inspector->asset_manager;

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
        if (igButton(
                "Add entity",
                (ImVec2){igGetContentRegionAvailWidth(), 30.0f})) {
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
        if (igButton(
                "Add asset", (ImVec2){igGetContentRegionAvailWidth(), 30.0f})) {
          // TODO: add asset
        }

        for (uint32_t i = 0; i < asset_manager->count; i++) {
          eg_asset_t *asset = eg_asset_manager_get(asset_manager, i);
          if (asset == NULL) {
            continue;
          }

          igPushIDInt(i);

          snprintf(
              str,
              sizeof(str),
              "%s: %s [%u]",
              EG_ASSET_NAMES[asset->type],
              eg_asset_get_name(asset),
              asset->index);
          if (igCollapsingHeader(str, 0)) {
            EG_ASSET_INSPECTORS[asset->type](asset, inspector);
          }

          igPopID();
          igSeparator();
        }

        igEndTabItem();
      }

      if (igBeginTabItem("Settings", NULL, 0)) {
        inspect_settings(inspector);

        igEndTabItem();
      }

      igEndTabBar();
    }
  }

  igEnd();

  if (!eg_world_exists(world, inspector->selected_entity)) {
    return;
  }

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

    igSeparator();

    if (igCollapsingHeader("Tags", ImGuiTreeNodeFlags_DefaultOpen)) {
      for (eg_tag_t tag = 0; tag < EG_TAG_MAX; tag++) {
        bool has_tag =
            EG_HAS_TAG(inspector->world, inspector->selected_entity, tag);
        igCheckbox(EG_TAG_NAMES[tag], &has_tag);
        EG_SET_TAG(inspector->world, inspector->selected_entity, tag, has_tag);
      }
    }

    for (uint32_t comp_id = 0; comp_id < EG_COMP_TYPE_MAX; comp_id++) {
      if (!EG_HAS_COMP_ID(world, comp_id, entity)) {
        continue;
      }

      igPushIDInt((int)comp_id);

      bool header_open = igCollapsingHeader(
          EG_COMP_NAMES[comp_id],
          ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap);

      igSameLine(igGetWindowWidth() - 25.0f, 0.0f);

      if (igSmallButton("×")) {
        eg_world_remove_comp(world, comp_id, entity);
        igPopID();
        continue;
      }

      if (header_open && EG_COMP_INSPECTORS[comp_id] != NULL) {
        EG_COMP_INSPECTORS[comp_id](
            EG_COMP_BY_ID(world, comp_id, inspector->selected_entity),
            inspector);
      }

      igPopID();
    }
  }

  igSeparator();
  add_component_button(inspector, inspector->selected_entity);

  igEnd();
}
