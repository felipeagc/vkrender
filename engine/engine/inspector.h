#pragma once

#include "world.h"

typedef struct re_window_t re_window_t;
typedef struct eg_world_t eg_world_t;
typedef struct eg_asset_manager_t eg_asset_manager_t;

typedef struct eg_inspector_t {
  eg_entity_t selected_entity;

  re_pipeline_t gizmo_pipeline;

  uint32_t pos_gizmo_index_count;
  re_buffer_t pos_gizmo_vertex_buffer;
  re_buffer_t pos_gizmo_index_buffer;
} eg_inspector_t;

void eg_inspector_init(
    eg_inspector_t *inspector);

void eg_inspector_draw_ui(
    eg_inspector_t *inspector,
    re_window_t *window,
    eg_world_t *world,
    eg_asset_manager_t *asset_manager);
