#pragma once

#include "common.hpp"
#include "window.hpp"

typedef struct re_imgui_t {
  re_window_t *window;
  VkDescriptorPool descriptor_pool;
} re_imgui_t;

void re_imgui_init(re_imgui_t *imgui, re_window_t *window);

void re_imgui_begin(re_imgui_t *imgui);

void re_imgui_end(re_imgui_t *imgui);

void re_imgui_draw(re_imgui_t *imgui);

void re_imgui_process_event(re_imgui_t *imgui, SDL_Event *event);

void re_imgui_destroy(re_imgui_t *imgui);
