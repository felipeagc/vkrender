#pragma once

#include "cmd_info.h"
#include <renderer/common.h>
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>

typedef struct re_window_t re_window_t;

void eg_imgui_init(re_window_t *window);

void eg_imgui_begin();

void eg_imgui_end();

void eg_imgui_draw(const eg_cmd_info_t *cmd_info);

void eg_imgui_mouse_button_callback(
    re_window_t *window, int button, int action, int mods);

void eg_imgui_scroll_callback(
    re_window_t *window, double xoffset, double yoffset);

void eg_imgui_key_callback(
    re_window_t *window, int key, int scancode, int action, int mods);

void eg_imgui_char_callback(re_window_t *window, unsigned int c);

void eg_imgui_destroy();
