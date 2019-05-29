#pragma once

#include <renderer/cmd_buffer.h>
#include <renderer/event.h>
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>

typedef struct re_window_t re_window_t;

void eg_imgui_init(re_window_t *window, re_render_target_t *render_target);

void eg_imgui_begin();

void eg_imgui_end();

void eg_imgui_draw(re_cmd_buffer_t *cmd_buffer);

void eg_imgui_process_event(const re_event_t *event);

void eg_imgui_destroy();
