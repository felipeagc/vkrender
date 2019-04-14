#pragma once

#include "../world.h"
#include <renderer/canvas.h>
#include <renderer/event.h>
#include <renderer/pipeline.h>

typedef enum eg_drag_direction_t {
  EG_DRAG_DIRECTION_NONE = 0,
  EG_DRAG_DIRECTION_X = UINT32_MAX - 1,
  EG_DRAG_DIRECTION_Y = UINT32_MAX - 2,
  EG_DRAG_DIRECTION_Z = UINT32_MAX - 3,
} eg_drag_direction_t;

typedef struct eg_picking_system_t {
  eg_world_t *world;
  re_window_t *window;

  re_render_target_t *drawing_render_target;

  re_canvas_t canvas;
  re_pipeline_t picking_pipeline;

  re_pipeline_t gizmo_pipeline;
  re_pipeline_t gizmo_picking_pipeline;

  uint32_t pos_gizmo_index_count;
  re_buffer_t pos_gizmo_vertex_buffer;
  re_buffer_t pos_gizmo_index_buffer;

  eg_drag_direction_t drag_direction;

  vec3_t pos_delta;
} eg_picking_system_t;

// render_target will be used to render the gizmos
void eg_picking_system_init(
    eg_picking_system_t *system,
    re_window_t *window,
    re_render_target_t *render_target,
    eg_world_t *world);

void eg_picking_system_destroy(eg_picking_system_t *system);

void eg_picking_system_process_event(
    eg_picking_system_t *system,
    const re_event_t *event,
    eg_entity_t *selected_entity);

void eg_picking_system_draw_gizmos(
    eg_picking_system_t *system,
    const eg_cmd_info_t *cmd_info,
    eg_entity_t entity);

void eg_picking_system_update(
    eg_picking_system_t *system, eg_entity_t selected_entity);
