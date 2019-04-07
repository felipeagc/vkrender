#pragma once

#include "common.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>

typedef struct re_window_t re_window_t;

void re_imgui_init(re_window_t *window);

void re_imgui_begin(re_window_t *window);

void re_imgui_end();

void re_imgui_draw(re_window_t *window);

void re_imgui_mouse_button_callback(
    re_window_t *window, int button, int action, int mods);

void re_imgui_scroll_callback(
    re_window_t *window, double xoffset, double yoffset);

void re_imgui_key_callback(
    re_window_t *window, int key, int scancode, int action, int mods);

void re_imgui_char_callback(re_window_t *window, unsigned int c);

void re_imgui_destroy();
