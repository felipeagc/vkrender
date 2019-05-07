#pragma once

#include "../cmd_info.h"
#include <renderer/window.h>

typedef struct eg_world_t eg_world_t;

void eg_rendering_system(eg_world_t *world, const eg_cmd_info_t *cmd_info);
