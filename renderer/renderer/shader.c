#include "shader.h"
#include "context.h"
#include "util.h"
#include <stdio.h>
#include <string.h>

void re_shader_init_spv(
    re_shader_t *shader, const uint32_t *code, size_t code_size) {
  shader->code = malloc(code_size);
  memcpy(shader->code, code, code_size);
  shader->code_size = code_size;

  VkShaderModuleCreateInfo create_info = {
      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      NULL,
      0,
      shader->code_size,
      shader->code};

  VK_CHECK(
      vkCreateShaderModule(g_ctx.device, &create_info, NULL, &shader->module));
}

void re_shader_destroy(re_shader_t *shader) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  if (shader->module != VK_NULL_HANDLE) {
    vkDestroyShaderModule(g_ctx.device, shader->module, NULL);
    shader->module = VK_NULL_HANDLE;
  }

  free(shader->code);
}
