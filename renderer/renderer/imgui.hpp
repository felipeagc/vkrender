#pragma once

#include "common.hpp"

union SDL_Event;

namespace renderer {
class Window;
}

struct re_imgui_t {
  renderer::Window *window;
  VkDescriptorPool descriptor_pool;
};

void re_imgui_init(re_imgui_t *imgui, renderer::Window *window);

void re_imgui_begin(re_imgui_t *imgui);

void re_imgui_end(re_imgui_t *imgui);

void re_imgui_draw(re_imgui_t *imgui);

void re_imgui_process_event(re_imgui_t *imgui, SDL_Event *event);

void re_imgui_destroy(re_imgui_t *imgui);
