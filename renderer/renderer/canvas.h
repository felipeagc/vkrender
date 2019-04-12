#pragma once

#include "cmd_buffer.h"
#include "common.h"
#include "pipeline.h"
#include "render_target.h"
#include <gmath.h>
#include <vulkan/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

typedef struct re_canvas_t {
  re_render_target_t render_target;

  VkClearColorValue clear_color;

  uint32_t width;
  uint32_t height;

  VkFormat depth_format;
  VkFormat color_format;

  // @NOTE: we could have multiple of these resources per frame-in-flight,
  // but from initial testing there's no real performance gain
  struct re_canvas_resource_t {
    struct {
      VkImage image;
      VmaAllocation allocation;
      // @TODO: no need for multiple samplers
      VkSampler sampler;
      VkImageView image_view;
    } color;

    struct {
      VkImage image;
      VmaAllocation allocation;
      VkImageView image_view;
    } depth;

    VkFramebuffer framebuffer;

    // For rendering this render target's image to another render target
    VkDescriptorSet descriptor_set;
  } resources[1];
} re_canvas_t;

void re_canvas_init(
    re_canvas_t *canvas,
    const uint32_t width,
    const uint32_t height,
    const VkFormat color_format);

void re_canvas_begin(re_canvas_t *canvas, re_cmd_buffer_t command_buffer);

void re_canvas_end(re_canvas_t *canvas, re_cmd_buffer_t command_buffer);

void re_canvas_draw(
    re_canvas_t *canvas,
    re_cmd_buffer_t command_buffer,
    re_pipeline_t *pipeline);

void re_canvas_resize(
    re_canvas_t *canvas, const uint32_t width, const uint32_t height);

void re_canvas_destroy(re_canvas_t *canvas);
