#pragma once

#include "cmd_buffer.h"
#include "image.h"
#include "pipeline.h"
#include "render_target.h"
#include "vulkan.h"
#include <gmath.h>
#include <vma/vk_mem_alloc.h>

typedef struct re_canvas_t {
  re_render_target_t render_target;

  VkClearColorValue clear_color;

  VkFormat depth_format;
  VkFormat color_format;

  uint32_t current_frame;

  struct {
    re_image_t color;
    re_image_t color_resolve;
    re_image_t depth;

    VkFramebuffer framebuffer;
  } resources[RE_MAX_FRAMES_IN_FLIGHT];
} re_canvas_t;

typedef struct re_canvas_options_t {
  uint32_t width;
  uint32_t height;
  VkFormat color_format;
  VkSampleCountFlags sample_count;
  VkClearColorValue clear_color;
} re_canvas_options_t;

void re_canvas_init(re_canvas_t *canvas, re_canvas_options_t *options);

void re_canvas_begin(re_canvas_t *canvas, re_cmd_buffer_t *cmd_buffer);

void re_canvas_end(re_canvas_t *canvas, re_cmd_buffer_t *cmd_buffer);

void re_canvas_draw(
    re_canvas_t *canvas, re_cmd_buffer_t *cmd_buffer, re_pipeline_t *pipeline);

void re_canvas_resize(
    re_canvas_t *canvas, const uint32_t width, const uint32_t height);

void re_canvas_destroy(re_canvas_t *canvas);
