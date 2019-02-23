#pragma once

#include <renderer/pipeline.hpp>
#include <util/file.h>

inline void eg_init_pipeline(
    re_pipeline_t *pipeline,
    const re_render_target_t render_target,
    const char *vertex_path,
    const char *fragment_path,
    const re_pipeline_parameters_t params) {
  re_shader_t shader;
  char *vertex_code = ut_load_string_from_file(vertex_path);
  char *fragment_code = ut_load_string_from_file(fragment_path);

  re_shader_init_glsl(
      &shader, vertex_path, vertex_code, fragment_path, fragment_code);

  re_pipeline_init_graphics(pipeline, render_target, shader, params);

  free(vertex_code);
  free(fragment_code);

  re_shader_destroy(&shader);
}

re_pipeline_parameters_t eg_standard_pipeline_parameters();

re_pipeline_parameters_t eg_billboard_pipeline_parameters();

re_pipeline_parameters_t eg_wireframe_pipeline_parameters();

re_pipeline_parameters_t eg_skybox_pipeline_parameters();

re_pipeline_parameters_t eg_fullscreen_pipeline_parameters();
