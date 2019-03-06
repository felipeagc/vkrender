#pragma once

#include <vulkan/vulkan.h>

typedef struct re_shader_t {
  VkShaderModule vertex_module;
  VkShaderModule fragment_module;
} re_shader_t;

void re_shader_init_compiler();

void re_shader_destroy_compiler();

bool re_shader_init_glsl(
    re_shader_t *shader,
    const char *vertex_path,
    const char *vertex_glsl_code,
    const char *fragment_path,
    const char *fragment_glsl_code);

void re_shader_init_spirv(
    re_shader_t *shader,
    const uint32_t *vertex_code,
    size_t vertex_code_size,
    const uint32_t *fragment_code,
    size_t fragment_code_size);

void re_shader_get_pipeline_stages(
    const re_shader_t *shader,
    VkPipelineShaderStageCreateInfo *pipeline_stages);

void re_shader_destroy(re_shader_t *shader);
