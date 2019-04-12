#pragma once

#include <renderer/window.h>

typedef struct re_pipeline_t re_pipeline_t;
typedef struct eg_world_t eg_world_t;

void eg_rendering_system_render(
    re_window_t *window,
    eg_world_t *world,
    VkCommandBuffer command_buffer,
    re_pipeline_t *pipeline);
