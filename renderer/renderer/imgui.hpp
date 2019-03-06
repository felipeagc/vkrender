#pragma once

#include "common.hpp"

void re_imgui_init(struct re_window_t *window);

void re_imgui_begin(struct re_window_t *window);

void re_imgui_end();

void re_imgui_draw(struct re_window_t *window);

void re_imgui_process_event(union SDL_Event *event);

void re_imgui_destroy();
