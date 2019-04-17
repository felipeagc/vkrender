#include "shader.h"
#include "context.h"
#include "util.h"
#include <stdio.h>
#include <string.h>

static inline VkShaderModule
create_shader_module(const uint32_t *code, size_t code_size) {
  VkShaderModule module;

  VkShaderModuleCreateInfo create_info = {
      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, NULL, 0, code_size, code};

  VK_CHECK(vkCreateShaderModule(g_ctx.device, &create_info, NULL, &module));
  return module;
}

void re_shader_init_spv(
    re_shader_t *shader, const uint32_t *code, size_t code_size) {
  shader->code = code;
  shader->code_size = code_size;

  shader->module = create_shader_module(code, code_size);
}

void re_shader_destroy(re_shader_t *shader) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  if (shader->module != VK_NULL_HANDLE) {
    vkDestroyShaderModule(g_ctx.device, shader->module, NULL);
    shader->module = VK_NULL_HANDLE;
  }
}
