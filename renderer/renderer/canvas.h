#pragma once

#include "cmd_buffer.h"
#include "pipeline.h"
#include "render_target.h"
#include <gmath.h>
#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

typedef struct re_canvas_t {
  re_render_target_t render_target;

  VkClearColorValue clear_color;

  VkFormat depth_format;
  VkFormat color_format;

  VkSampler sampler;

  struct {
    VkImage image;
    VmaAllocation allocation;
    VkImageView image_view;
  } color;

  struct {
    VkImage image;
    VmaAllocation allocation;
    VkImageView image_view;
  } resolve;

  struct {
    VkImage image;
    VmaAllocation allocation;
    VkImageView image_view;
  } depth;

  VkFramebuffer framebuffer;
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
