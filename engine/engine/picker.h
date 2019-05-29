#pragma once

#include <renderer/buffer.h>
#include <renderer/canvas.h>
#include <renderer/cmd_buffer.h>

typedef struct eg_picker_t {
  re_window_t *window;
  re_canvas_t canvas;
  re_buffer_t pixel_buffer;
  re_cmd_buffer_t cmd_buffer;
  VkFence fence;
} eg_picker_t;

void eg_picker_init(
    eg_picker_t *picker, re_window_t *window, uint32_t width, uint32_t height);

void eg_picker_destroy(eg_picker_t *picker);

void eg_picker_resize(eg_picker_t *picker, uint32_t width, uint32_t height);

re_cmd_buffer_t *eg_picker_begin(eg_picker_t *picker);

void eg_picker_end(eg_picker_t *picker);

uint32_t eg_picker_get(eg_picker_t *picker, uint32_t x, uint32_t y);
