#pragma once

#include "buffer.h"

typedef struct re_buffer_pool_node_t {
  re_buffer_t buffer;
  void *mapping;

  bool *in_use;

  struct re_buffer_pool_node_t *next;
} re_buffer_pool_node_t;

typedef struct re_buffer_pool_t {
  re_buffer_options_t buffer_options;
  uint32_t buffer_parts;
  uint32_t part_size;

  uint32_t current_frame;

  re_buffer_pool_node_t base_nodes[RE_MAX_FRAMES_IN_FLIGHT];
} re_buffer_pool_t;

void re_buffer_pool_init(
    re_buffer_pool_t *buffer_pool, re_buffer_options_t *options);

void re_buffer_pool_begin_frame(re_buffer_pool_t *buffer_pool);

void *re_buffer_pool_alloc(
    re_buffer_pool_t *buffer_pool,
    size_t size,
    re_descriptor_info_t *out_descriptor);

void re_buffer_pool_destroy(re_buffer_pool_t *buffer_pool);
