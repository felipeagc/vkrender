#pragma once

#include <vulkan/vulkan.h>

typedef struct re_shader_t {
  VkShaderModule module;
  uint32_t *code;
  size_t code_size;
} re_shader_t;

void re_shader_init_spv(
    re_shader_t *shader, uint32_t *code, size_t code_size);

void re_shader_destroy(re_shader_t *shader);
