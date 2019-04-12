#pragma once

#include "../world.h"
#include <renderer/canvas.h>
#include <renderer/pipeline.h>

typedef struct eg_picking_system_t {
  re_canvas_t canvas;
  re_pipeline_t picking_pipeline;
} eg_picking_system_t;

void eg_picking_system_init(
    eg_picking_system_t *system, uint32_t width, uint32_t height);

void eg_picking_system_destroy(eg_picking_system_t *system);

void eg_picking_system_resize(
    eg_picking_system_t *system, uint32_t width, uint32_t height);

eg_entity_t eg_picking_system_pick(
    eg_picking_system_t *system,
    uint32_t frame_index,
    eg_world_t *world,
    uint32_t mouse_x,
    uint32_t mouse_y);
