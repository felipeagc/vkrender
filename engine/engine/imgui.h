#pragma once

#include "cmd_info.h"
#include <renderer/common.h>
#include <renderer/event.h>
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>

typedef struct re_window_t re_window_t;

void eg_imgui_init(re_window_t *window);

void eg_imgui_begin();

void eg_imgui_end();

void eg_imgui_draw(const eg_cmd_info_t *cmd_info);

void eg_imgui_process_event(const re_event_t *event);

void eg_imgui_destroy();
