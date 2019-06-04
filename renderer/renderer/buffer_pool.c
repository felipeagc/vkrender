#include "buffer_pool.h"

#include "context.h"

static void
node_init(re_buffer_pool_t *buffer_pool, re_buffer_pool_node_t *node) {
  node->next = NULL;

  re_buffer_init(&node->buffer, &buffer_pool->buffer_options);
  re_buffer_map_memory(&node->buffer, &node->mapping);

  node->in_use = calloc(sizeof(*node->in_use), buffer_pool->buffer_parts);

  for (uint32_t i = 0; i < buffer_pool->buffer_parts; i++) {
    node->in_use[i] = false;
  }
}

static void node_destroy(re_buffer_pool_node_t *node) {
  re_buffer_unmap_memory(&node->buffer);
  re_buffer_destroy(&node->buffer);

  free(node->in_use);
}

void re_buffer_pool_init(
    re_buffer_pool_t *buffer_pool, re_buffer_options_t *options) {
  buffer_pool->buffer_options = *options;
  buffer_pool->current_frame = 0;
  buffer_pool->part_size =
      (uint32_t)g_ctx.physical_limits.minUniformBufferOffsetAlignment;
  buffer_pool->buffer_parts = (uint32_t)options->size / buffer_pool->part_size;
  assert(buffer_pool->buffer_parts > 1);

  for (uint32_t i = 0; i < RE_MAX_FRAMES_IN_FLIGHT; i++) {
    node_init(buffer_pool, &buffer_pool->base_nodes[i]);
  }
}

void re_buffer_pool_begin_frame(re_buffer_pool_t *buffer_pool) {
  buffer_pool->current_frame =
      (buffer_pool->current_frame + 1) % RE_MAX_FRAMES_IN_FLIGHT;
  re_buffer_pool_node_t *node =
      &buffer_pool->base_nodes[buffer_pool->current_frame];
  while (node != NULL) {
    for (uint32_t i = 0; i < buffer_pool->buffer_parts; i++) {
      node->in_use[i] = false;
    }
    node = node->next;
  }
}

void *re_buffer_pool_alloc(
    re_buffer_pool_t *buffer_pool,
    size_t size,
    re_descriptor_info_t *out_descriptor,
    uint32_t *out_offset) {
  uint32_t required_parts =
      ((uint32_t)size + buffer_pool->part_size - 1) / buffer_pool->part_size;

  re_buffer_pool_node_t *node =
      &buffer_pool->base_nodes[buffer_pool->current_frame];
  while (node != NULL) {
    for (uint32_t i = 0; i < buffer_pool->buffer_parts - required_parts + 1;
         i++) {
      bool good = true;
      for (uint32_t j = 0; j < required_parts; j++) {
        if (node->in_use[i + j]) {
          good = false;
        }
      }

      if (good) {
        for (uint32_t j = 0; j < required_parts; j++) {
          node->in_use[i] = true;
        }
        *out_descriptor = (re_descriptor_info_t){
            .buffer = {.buffer = node->buffer.buffer,
                       .offset = 0,
                       .range = VK_WHOLE_SIZE},
        };
        *out_offset = i * buffer_pool->part_size;
        return ((uint8_t *)node->mapping) + i * buffer_pool->part_size;
      }
    }

    if (node->next == NULL) {
      // TODO: replace this malloc with something faster
      node->next = calloc(sizeof(re_buffer_pool_node_t), 1);
      node_init(buffer_pool, node->next);
    }

    node = node->next;
  }

  return NULL;
}

void re_buffer_pool_destroy(re_buffer_pool_t *buffer_pool) {
  for (uint32_t i = 0; i < RE_MAX_FRAMES_IN_FLIGHT; i++) {
    re_buffer_pool_node_t *node = &buffer_pool->base_nodes[i];

    while (node != NULL) {
      node_destroy(node);
      node = node->next;
    }

    node = buffer_pool->base_nodes[i].next;

    while (node != NULL) {
      re_buffer_pool_node_t *old_node = node;
      node = node->next;
      free(old_node);
    }
  }
}
