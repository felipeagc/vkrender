#pragma once

#include <fstd/file.h>
#include <renderer/pipeline.hpp>

inline bool eg_init_pipeline_spv(
    re_pipeline_t *pipeline,
    const re_render_target_t render_target,
    const char *vertex_path,
    const char *fragment_path,
    const re_pipeline_parameters_t params) {
  size_t vertex_size;
  unsigned char *vertex_code =
      fstd_load_bytes_from_file(vertex_path, &vertex_size);
  size_t fragment_size;
  unsigned char *fragment_code =
      fstd_load_bytes_from_file(fragment_path, &fragment_size);

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
