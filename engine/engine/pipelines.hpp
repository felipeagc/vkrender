#pragma once

#include <fstd/file.h>
#include <renderer/pipeline.hpp>

inline bool eg_init_shader_and_pipeline(
    re_shader_t *shader,
    re_pipeline_t *pipeline,
    const re_render_target_t render_target,
    const char *vertex_path,
    const char *fragment_path,
    const re_pipeline_parameters_t params) {
  char *vertex_code = fstd_load_string_from_file(vertex_path);
  char *fragment_code = fstd_load_string_from_file(fragment_path);

  if (!re_shader_init_glsl(
          shader, vertex_path, vertex_code, fragment_path, fragment_code)) {
    return false;
  }

  re_pipeline_init_graphics(pipeline, render_target, shader, params);

  free(vertex_code);
  free(fragment_code);

  return true;
}

re_pipeline_parameters_t eg_pbr_pipeline_parameters();

re_pipeline_parameters_t eg_billboard_pipeline_parameters();

re_pipeline_parameters_t eg_wireframe_pipeline_parameters();

re_pipeline_parameters_t eg_skybox_pipeline_parameters();

re_pipeline_parameters_t eg_fullscreen_pipeline_parameters();
