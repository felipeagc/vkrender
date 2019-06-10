#pragma once

#include "pipeline.h"
#include "util.h"

typedef struct re_buffer_t re_buffer_t;
typedef struct re_image_t re_image_t;

typedef VkCommandPool re_cmd_pool_t;

typedef struct re_cmd_buffer_t {
  VkCommandBuffer cmd_buffer;

  re_descriptor_info_t bindings[RE_MAX_DESCRIPTOR_SETS]
                               [RE_MAX_DESCRIPTOR_SET_BINDINGS];
  uint32_t dynamic_offset;

  re_viewport_t viewport;
  re_rect_2d_t scissor;
} re_cmd_buffer_t;

typedef enum re_cmd_buffer_usage_t {
  RE_CMD_BUFFER_USAGE_ONE_TIME_SUBMIT =
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  RE_CMD_BUFFER_USAGE_RENDER_PASS_CONTINUE =
      VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
  RE_CMD_BUFFER_SIMULTANEOUS_USE = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
} re_cmd_buffer_usage_t;

typedef enum re_cmd_buffer_level_t {
  RE_CMD_BUFFER_LEVEL_PRIMARY   = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
  RE_CMD_BUFFER_LEVEL_SECONDARY = VK_COMMAND_BUFFER_LEVEL_SECONDARY,
} re_cmd_buffer_level_t;

typedef struct re_cmd_buffer_alloc_info_t {
  re_cmd_pool_t pool;
  uint32_t count;
  re_cmd_buffer_level_t level;
} re_cmd_buffer_alloc_info_t;

typedef struct re_cmd_buffer_begin_info_t {
  re_cmd_buffer_usage_t usage;
} re_cmd_buffer_begin_info_t;

void re_allocate_cmd_buffers(
    re_cmd_buffer_alloc_info_t *alloc_info, re_cmd_buffer_t *buffers);

void re_free_cmd_buffers(
    re_cmd_pool_t pool, uint32_t buffer_count, re_cmd_buffer_t *buffers);

void re_begin_cmd_buffer(
    re_cmd_buffer_t *cmd_buffer, re_cmd_buffer_begin_info_t *begin_info);

void re_end_cmd_buffer(re_cmd_buffer_t *cmd_buffer);

/*
 *
 * Commands
 *
 */

void re_cmd_bind_pipeline(re_cmd_buffer_t *cmd_buffer, re_pipeline_t *pipeline);

void re_cmd_bind_descriptor_set(
    re_cmd_buffer_t *cmd_buffer, re_pipeline_t *pipeline, uint32_t set_index);

void re_cmd_bind_descriptor(
    re_cmd_buffer_t *cmd_buffer,
    uint32_t binding,
    uint32_t set,
    re_descriptor_info_t descriptor);

void re_cmd_bind_image(
    re_cmd_buffer_t *cmd_buffer,
    uint32_t set,
    uint32_t binding,
    re_image_t *image);

void *re_cmd_bind_uniform(
    re_cmd_buffer_t *cmd_buffer, uint32_t set, uint32_t binding, size_t size);

// TODO: push constants
void re_cmd_push_constants(
    re_cmd_buffer_t *cmd_buffer,
    re_pipeline_t *pipeline,
    uint32_t index,
    uint32_t size,
    const void *values);

// TODO: render pass

// TODO: copy buffer, etc

// TODO: clear attachments

void re_cmd_set_viewport(re_cmd_buffer_t *cmd_buffer, re_viewport_t *viewport);

void re_cmd_set_scissor(re_cmd_buffer_t *cmd_buffer, re_rect_2d_t *rect);

void re_cmd_draw(
    re_cmd_buffer_t *cmd_buffer,
    uint32_t vertex_count,
    uint32_t instance_count,
    uint32_t first_vertex,
    uint32_t first_instance);

void re_cmd_draw_indexed(
    re_cmd_buffer_t *cmd_buffer,
    uint32_t index_count,
    uint32_t instance_count,
    uint32_t first_index,
    int32_t vertex_offset,
    uint32_t first_instance);

void re_cmd_bind_vertex_buffers(
    re_cmd_buffer_t *cmd_buffer,
    uint32_t first_binding,
    uint32_t binding_count,
    re_buffer_t *buffers,
    const size_t *offsets);

void re_cmd_bind_index_buffer(
    re_cmd_buffer_t *cmd_buffer,
    re_buffer_t *buffer,
    size_t offset,
    re_index_type_t index_type);
