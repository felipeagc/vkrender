#pragma once

#include "world.h"

typedef struct re_window_t re_window_t;
typedef struct eg_world_t eg_world_t;
typedef struct eg_asset_manager_t eg_asset_manager_t;

typedef struct eg_inspector_t {
  eg_entity_t selected_entity;
} eg_inspector_t;

void eg_inspector_init(eg_inspector_t *inspector);

void eg_draw_inspector(
    eg_inspector_t *inspector,
    re_window_t *window,
    eg_world_t *world,
    eg_asset_manager_t *asset_manager);
