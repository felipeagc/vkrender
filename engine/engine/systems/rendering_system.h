#pragma once

#include <renderer/window.h>

typedef struct eg_world_t eg_world_t;

void eg_rendering_system(eg_world_t *world, re_cmd_buffer_t *cmd_buffer);
