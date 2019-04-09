#pragma once

typedef struct re_window_t re_window_t;
typedef struct eg_world_t eg_world_t;
typedef struct eg_asset_manager_t eg_asset_manager_t;

void eg_draw_inspector(
    re_window_t *window, eg_world_t *world, eg_asset_manager_t *asset_manager);
