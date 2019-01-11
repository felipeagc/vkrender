#pragma once

#include <renderer/pipeline.hpp>
#include <util/file.hpp>

inline void eg_init_pipeline(
    renderer::GraphicsPipeline *pipeline,
    renderer::RenderTarget *render_target,
    const char *vertex_path,
    const char *fragment_path,
    const renderer::PipelineParameters params) {
  re_shader_t shader;
  char *vertex_code = load_string_from_file(vertex_path);
  char *fragment_code = load_string_from_file(fragment_path);

  re_shader_init_glsl(&shader, vertex_code, fragment_code);

  *pipeline = renderer::GraphicsPipeline(*render_target, shader, params);

  free(vertex_code);
  free(fragment_code);

  re_shader_destroy(&shader);
}
