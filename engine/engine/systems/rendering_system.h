#pragma once

#include <renderer/window.h>

typedef struct eg_scene_t eg_scene_t;

void eg_rendering_system(eg_scene_t *scene, re_cmd_buffer_t *cmd_buffer);
