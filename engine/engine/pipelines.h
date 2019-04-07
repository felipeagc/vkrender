#pragma once

#include "filesystem.h"
#include <renderer/pipeline.h>
#include <stdbool.h>

static inline bool eg_init_pipeline_spv(
    re_pipeline_t *pipeline,
    const re_render_target_t *render_target,
    const char *vertex_path,
    const char *fragment_path,
    const re_pipeline_parameters_t params) {
  eg_file_t *vertex_file = eg_file_open_read(vertex_path);
  assert(vertex_file);
  size_t vertex_size = eg_file_size(vertex_file);
  unsigned char *vertex_code = calloc(1, vertex_size);
  eg_file_read_bytes(vertex_file, vertex_code, vertex_size);
  eg_file_close(vertex_file);

  eg_file_t *fragment_file = eg_file_open_read(fragment_path);
  assert(fragment_file);
  size_t fragment_size = eg_file_size(fragment_file);
  unsigned char *fragment_code = calloc(1, fragment_size);
  eg_file_read_bytes(fragment_file, fragment_code, fragment_size);
  eg_file_close(fragment_file);

  re_shader_t shader;
  re_shader_init_spv(
      &shader,
      (uint32_t *)vertex_code,
      vertex_size,
      (uint32_t *)fragment_code,
      fragment_size);

  re_pipeline_init_graphics(pipeline, render_target, &shader, params);

  free(vertex_code);
  free(fragment_code);

  re_shader_destroy(&shader);

  return true;
}

re_pipeline_parameters_t eg_pbr_pipeline_parameters();

re_pipeline_parameters_t eg_billboard_pipeline_parameters();

re_pipeline_parameters_t eg_wireframe_pipeline_parameters();

re_pipeline_parameters_t eg_skybox_pipeline_parameters();

re_pipeline_parameters_t eg_fullscreen_pipeline_parameters();
