#pragma once

#include "../world.h"
#include <renderer/canvas.h>
#include <renderer/pipeline.h>

typedef enum eg_drag_direction_t {
  EG_DRAG_DIRECTION_NONE = 0,
  EG_DRAG_DIRECTION_X = UINT32_MAX - 1,
  EG_DRAG_DIRECTION_Y = UINT32_MAX - 2,
  EG_DRAG_DIRECTION_Z = UINT32_MAX - 3,
} eg_drag_direction_t;

typedef struct eg_picking_system_t {
  re_canvas_t canvas;
  re_pipeline_t picking_pipeline;

  re_pipeline_t gizmo_pipeline;
  re_pipeline_t gizmo_picking_pipeline;

  uint32_t pos_gizmo_index_count;
  re_buffer_t pos_gizmo_vertex_buffer;
  re_buffer_t pos_gizmo_index_buffer;

  eg_drag_direction_t drag_direction;

  bool left_pressed;
  vec3_t pos_delta;
} eg_picking_system_t;

// render_target will be used to render the gizmos
void eg_picking_system_init(
    eg_picking_system_t *system,
    re_render_target_t *render_target,
    uint32_t width,
    uint32_t height);

void eg_picking_system_destroy(eg_picking_system_t *system);

void eg_picking_system_resize(
    eg_picking_system_t *system, uint32_t width, uint32_t height);

void eg_picking_system_draw_gizmos(
    eg_picking_system_t *system,
    eg_world_t *world,
    eg_entity_t entity,
    const eg_cmd_info_t *cmd_info,
    uint32_t width,
    uint32_t height);

void eg_picking_system_mouse_press(
    eg_picking_system_t *system,
    eg_world_t *world,
    eg_entity_t *selected_entity,
    uint32_t frame_index,
    uint32_t mouse_x,
    uint32_t mouse_y);

void eg_picking_system_mouse_release(eg_picking_system_t *system);

void eg_picking_system_update(
    eg_picking_system_t *system,
    re_window_t *window,
    eg_world_t *world,
    eg_entity_t selected_entity,
    uint32_t width,
    uint32_t height);
