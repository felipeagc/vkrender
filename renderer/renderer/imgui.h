#pragma once

#include "common.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>

typedef struct re_window_t re_window_t;
typedef union SDL_Event SDL_Event;

void re_imgui_init(re_window_t *window);

void re_imgui_begin(re_window_t *window);

void re_imgui_end();

void re_imgui_draw(re_window_t *window);

void re_imgui_process_event(SDL_Event *event);

void re_imgui_destroy();