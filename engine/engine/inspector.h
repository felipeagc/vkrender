#pragma once

#include "picker.h"
#include "world.h"
#include <renderer/canvas.h>
#include <renderer/event.h>
#include <renderer/pipeline.h>

typedef struct re_window_t re_window_t;
typedef struct eg_world_t eg_world_t;
typedef struct eg_asset_manager_t eg_asset_manager_t;

typedef enum eg_drag_direction_t {
  EG_DRAG_DIRECTION_NONE = 0,
  EG_DRAG_DIRECTION_X = UINT32_MAX - 1,
  EG_DRAG_DIRECTION_Y = UINT32_MAX - 2,
  EG_DRAG_DIRECTION_Z = UINT32_MAX - 3,
} eg_drag_direction_t;

typedef struct eg_inspector_t {
  eg_entity_t selected_entity;

  re_window_t *window;
  eg_world_t *world;
  eg_asset_manager_t *asset_manager;

  re_image_t light_billboard_image;

  eg_picker_t picker;

  re_render_target_t *drawing_render_target;

  re_pipeline_t picking_pipeline;

  re_pipeline_t gizmo_pipeline;
  re_pipeline_t gizmo_picking_pipeline;

  re_pipeline_t billboard_pipeline;
  re_pipeline_t billboard_picking_pipeline;

  re_pipeline_t outline_pipeline;

  uint32_t pos_gizmo_index_count;
  re_buffer_t pos_gizmo_vertex_buffer;
  re_buffer_t pos_gizmo_index_buffer;

  eg_drag_direction_t drag_direction;

  vec3_t pos_delta;

  float snapping;
  bool snap;
} eg_inspector_t;

void eg_inspector_init(
    eg_inspector_t *inspector,
    re_window_t *window,
    re_render_target_t *render_target,
    eg_world_t *world,
    eg_asset_manager_t *asset_manager);

void eg_inspector_destroy(eg_inspector_t *inspector);

void eg_inspector_process_event(
    eg_inspector_t *inspector, const re_event_t *event);

void eg_inspector_update(eg_inspector_t *inspector);

void eg_inspector_draw_gizmos(
    eg_inspector_t *inspector, re_cmd_buffer_t *cmd_buffer);

void eg_inspector_draw_selected_outline(
    eg_inspector_t *inspector, re_cmd_buffer_t *cmd_buffer);

void eg_inspector_draw_ui(eg_inspector_t *inspector);

