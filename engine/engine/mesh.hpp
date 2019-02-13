#pragma once

#include "pbr.hpp"
#include <renderer/buffer.hpp>
#include <renderer/pipeline.hpp>

typedef struct eg_mesh_t {
  re_buffer_t vertex_buffer;
  re_buffer_t index_buffer;
  uint32_t index_count;
} eg_mesh_t;

void eg_mesh_init(
    eg_mesh_t *mesh,
    re_vertex_t *vertices,
    uint32_t vertex_count,
    uint32_t *indices,
    uint32_t index_count);

void eg_mesh_draw(eg_mesh_t *mesh, struct re_window_t *window);

void eg_mesh_destroy(eg_mesh_t *mesh);
