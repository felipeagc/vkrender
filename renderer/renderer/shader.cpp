#include "shader.hpp"
#include "context.hpp"
#include "util.hpp"
#include <stdio.h>
#include <string.h>
#include <util/log.h>

static inline VkShaderModule
create_shader_module(const uint32_t *code, size_t code_size) {
  VkShaderModule module;

  VkShaderModuleCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, NULL, 0, code_size, code};

  VK_CHECK(vkCreateShaderModule(g_ctx.device, &createInfo, NULL, &module));
  return module;
}

void re_shader_init_spv(
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

  if (shader->vertex_module != VK_NULL_HANDLE) {
    vkDestroyShaderModule(g_ctx.device, shader->vertex_module, NULL);
    shader->vertex_module = VK_NULL_HANDLE;
  }

  if (shader->fragment_module != VK_NULL_HANDLE) {
    vkDestroyShaderModule(g_ctx.device, shader->fragment_module, NULL);
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
