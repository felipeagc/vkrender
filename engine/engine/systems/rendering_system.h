#pragma once

#include "../cmd_info.h"
#include <renderer/window.h>

typedef struct re_pipeline_t re_pipeline_t;
typedef struct eg_world_t eg_world_t;

void eg_rendering_system_render(
    const eg_cmd_info_t *cmd_info,
    eg_world_t *world,
    re_pipeline_t *pipeline);
