#pragma once

#include <renderer/pipeline.hpp>
#include <util/file.hpp>

inline void eg_init_pipeline(
    re_pipeline_t *pipeline,
    const re_render_target_t render_target,
    const char *vertex_path,
    const char *fragment_path,
    const re_pipeline_parameters_t params) {
  re_shader_t shader;
  char *vertex_code = load_string_from_file(vertex_path);
  char *fragment_code = load_string_from_file(fragment_path);

  re_shader_init_glsl(&shader, vertex_code, fragment_code);

  re_pipeline_init_graphics(pipeline, render_target, shader, params);

  free(vertex_code);
  free(fragment_code);

  re_shader_destroy(&shader);
}
