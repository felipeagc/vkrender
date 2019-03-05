#include "shader.hpp"
#include "context.hpp"
#include "util.hpp"
#include <shaderc/shaderc.h>
#include <stdio.h>
#include <string.h>
#include <util/log.h>

static shaderc_compiler_t g_compiler;

static inline VkShaderModule
create_shader_module(const uint32_t *code, size_t code_size) {
  VkShaderModule module;

  VkShaderModuleCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, NULL, 0, code_size, code};

  VK_CHECK(vkCreateShaderModule(g_ctx.device, &createInfo, NULL, &module));
  return module;
}

void re_shader_init_compiler() { g_compiler = shaderc_compiler_initialize(); }

void re_shader_destroy_compiler() { shaderc_compiler_release(g_compiler); }

bool re_shader_init_glsl(
    re_shader_t *shader,
    const char *vertex_path,
    const char *vertex_glsl_code,
    const char *fragment_path,
    const char *fragment_glsl_code) {
  shaderc_compile_options_t options = shaderc_compile_options_initialize();
  shaderc_compile_options_set_optimization_level(
      options, shaderc_optimization_level_performance);

  // On the same, other or multiple simultaneous threads.
  shaderc_compilation_result_t vertex_result = shaderc_compile_into_spv(
      g_compiler,
      vertex_glsl_code,
      strlen(vertex_glsl_code),
      shaderc_glsl_vertex_shader,
      vertex_path,
      "main",
      options);

  if (shaderc_result_get_compilation_status(vertex_result) !=
      shaderc_compilation_status_success) {
    ut_log_error(
        "Failed to compile vertex shader:\n%s\n",
        shaderc_result_get_error_message(vertex_result));
    shaderc_result_release(vertex_result);
    shaderc_compile_options_release(options);
    return false;
  }

  shaderc_compilation_result_t fragment_result = shaderc_compile_into_spv(
      g_compiler,
      fragment_glsl_code,
      strlen(fragment_glsl_code),
      shaderc_glsl_fragment_shader,
      fragment_path,
      "main",
      options);

  if (shaderc_result_get_compilation_status(fragment_result) !=
      shaderc_compilation_status_success) {
    ut_log_error(
        "Failed to compile fragment shader:\n%s\n",
        shaderc_result_get_error_message(fragment_result));
    shaderc_result_release(vertex_result);
    shaderc_result_release(fragment_result);
    shaderc_compile_options_release(options);
    return false;
  }

  shader->vertex_module = create_shader_module(
      (uint32_t *)shaderc_result_get_bytes(vertex_result),
      shaderc_result_get_length(vertex_result));
  shader->fragment_module = create_shader_module(
      (uint32_t *)shaderc_result_get_bytes(fragment_result),
      shaderc_result_get_length(fragment_result));

  shaderc_result_release(vertex_result);
  shaderc_result_release(fragment_result);
  shaderc_compile_options_release(options);

  return true;
}

void re_shader_init_spirv(
    re_shader_t *shader,
    const uint32_t *vertex_code,
    size_t vertex_code_size,
    const uint32_t *fragment_code,
    size_t fragment_code_size) {
  shader->vertex_module = create_shader_module(vertex_code, vertex_code_size);
  shader->fragment_module =
      create_shader_module(fragment_code, fragment_code_size);
}

void re_shader_destroy(re_shader_t *shader) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  if (shader->vertex_module != VK_NULL_HANDLE &&
      shader->fragment_module != VK_NULL_HANDLE) {
    vkDestroyShaderModule(g_ctx.device, shader->vertex_module, NULL);
    vkDestroyShaderModule(g_ctx.device, shader->fragment_module, NULL);
    shader->vertex_module = VK_NULL_HANDLE;
    shader->fragment_module = VK_NULL_HANDLE;
  }
}

void re_shader_get_pipeline_stages(
    const re_shader_t *shader,
    VkPipelineShaderStageCreateInfo *pipeline_stages) {
  pipeline_stages[0] = {
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // sType
      NULL,                                                // pNext
      0,                                                   // flags
      VK_SHADER_STAGE_VERTEX_BIT,                          // stage
      shader->vertex_module,                               // module
      "main",                                              // pName
      NULL, // pSpecializationInfo
  };
  pipeline_stages[1] = {
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // sType
      NULL,                                                // pNext
      0,                                                   // flags
      VK_SHADER_STAGE_FRAGMENT_BIT,                        // stage
      shader->fragment_module,                             // module
      "main",                                              // pName
      NULL, // pSpecializationInfo
  };
}
