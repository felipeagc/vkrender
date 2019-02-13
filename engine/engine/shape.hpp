#pragma once

#include "pbr.hpp"
#include <renderer/buffer.hpp>
#include <renderer/pipeline.hpp>

typedef struct eg_shape_t {
  re_buffer_t vertex_buffer;
  re_buffer_t index_buffer;
  uint32_t index_count;
} eg_shape_t;

void eg_shape_init(
    eg_shape_t *shape,
    re_vertex_t *vertices,
    uint32_t vertex_count,
    uint32_t *indices,
    uint32_t index_count);

void eg_shape_draw(eg_shape_t *shape, struct re_window_t *window);

void eg_shape_destroy(eg_shape_t *shape);
