#pragma once

#include "pipeline.h"
#include "util.h"

typedef VkCommandPool re_cmd_pool_t;
typedef VkCommandBuffer re_cmd_buffer_t;

typedef enum re_cmd_buffer_usage_t {
  RE_CMD_BUFFER_USAGE_ONE_TIME_SUBMIT =
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  RE_CMD_BUFFER_USAGE_RENDER_PASS_CONTINUE =
      VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
  RE_CMD_BUFFER_SIMULTANEOUS_USE = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
} re_cmd_buffer_usage_t;

typedef enum re_cmd_buffer_level_t {
  RE_CMD_BUFFER_LEVEL_PRIMARY = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
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
    re_cmd_buffer_t cmd_buffer, re_cmd_buffer_begin_info_t *begin_info);

void re_end_cmd_buffer(re_cmd_buffer_t cmd_buffer);

/*
 *
 * Commands
 *
 */

void re_cmd_bind_graphics_pipeline(
    re_cmd_buffer_t cmd_buffer, re_pipeline_t *pipeline);
