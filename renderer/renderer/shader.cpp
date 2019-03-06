#include "shader.hpp"
#include "context.hpp"
#include "util.hpp"
#include <shaderc/shaderc.h>
#include <spirv_reflect/spirv_reflect.h>
#include <stdio.h>
#include <string.h>
#include <util/log.h>

static shaderc_compiler_t g_compiler;

typedef struct {
  uint32_t set_index;
  uint32_t binding_count;
  VkDescriptorSetLayoutCreateInfo create_info;
  VkDescriptorSetLayoutBinding *bindings;
} descriptor_set_layout_data_t;

static inline void get_descriptor_set_layout_data_from_code(
    const uint32_t *code,
    size_t code_nbytes,
    uint32_t *set_layout_count,
    descriptor_set_layout_data_t *set_layouts) {
  SpvReflectShaderModule module = {};
  SpvReflectResult result =
      spvReflectCreateShaderModule(code_nbytes, code, &module);
  assert(result == SPV_REFLECT_RESULT_SUCCESS);

  *set_layout_count = 0;
  result = spvReflectEnumerateDescriptorSets(&module, set_layout_count, NULL);
  assert(result == SPV_REFLECT_RESULT_SUCCESS);

  SpvReflectDescriptorSet **sets = (SpvReflectDescriptorSet **)malloc(
      sizeof(SpvReflectDescriptorSet *) * (*set_layout_count));
  result = spvReflectEnumerateDescriptorSets(&module, set_layout_count, sets);
  assert(result == SPV_REFLECT_RESULT_SUCCESS);

  if (set_layouts != NULL) {
    for (uint32_t i_set = 0; i_set < *set_layout_count; i_set++) {
      set_layouts[i_set] = {};

      const SpvReflectDescriptorSet *refl_set = sets[i_set];
      descriptor_set_layout_data_t *layout = &set_layouts[i_set];
      layout->binding_count = refl_set->binding_count;
      layout->bindings = (VkDescriptorSetLayoutBinding *)calloc(
          refl_set->binding_count, sizeof(VkDescriptorSetLayoutBinding));

      for (uint32_t i_binding = 0; i_binding < refl_set->binding_count;
           i_binding++) {
        const SpvReflectDescriptorBinding *refl_binding =
            refl_set->bindings[i_binding];

        VkDescriptorSetLayoutBinding *layout_binding =
            &layout->bindings[i_binding];
        layout_binding->binding = refl_binding->binding;
        layout_binding->descriptorType =
            (VkDescriptorType)refl_binding->descriptor_type;
        layout_binding->descriptorCount = 1;

        for (uint32_t i_dim = 0; i_dim < refl_binding->array.dims_count;
             i_dim++) {
          layout_binding->descriptorCount *= refl_binding->array.dims[i_dim];
        }

        layout_binding->stageFlags = (VkShaderStageFlagBits)module.shader_stage;
        layout_binding->pImmutableSamplers = NULL;
      }

      layout->set_index = refl_set->set;
      layout->create_info.sType =
          VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      layout->create_info.bindingCount = refl_set->binding_count;
      layout->create_info.pBindings = layout->bindings;
    }
  }

  free(sets);

  spvReflectDestroyShaderModule(&module);
}

static inline void create_descriptor_set_layouts(
    const uint32_t *vertex_code,
    size_t vertex_code_size,
    const uint32_t *fragment_code,
    size_t fragment_code_size,
    uint32_t *descriptor_set_layout_count,
    VkDescriptorSetLayout **descriptor_set_layouts) {
  uint32_t vertex_set_layout_count;
  get_descriptor_set_layout_data_from_code(
      vertex_code, vertex_code_size, &vertex_set_layout_count, NULL);
  descriptor_set_layout_data_t *vertex_set_layouts =
      (descriptor_set_layout_data_t *)malloc(
          sizeof(descriptor_set_layout_data_t) * vertex_set_layout_count);
  get_descriptor_set_layout_data_from_code(
      vertex_code,
      vertex_code_size,
      &vertex_set_layout_count,
      vertex_set_layouts);

  uint32_t fragment_set_layout_count;
  get_descriptor_set_layout_data_from_code(
      fragment_code, fragment_code_size, &fragment_set_layout_count, NULL);
  descriptor_set_layout_data_t *fragment_set_layouts =
      (descriptor_set_layout_data_t *)malloc(
          sizeof(descriptor_set_layout_data_t) * fragment_set_layout_count);
  get_descriptor_set_layout_data_from_code(
      fragment_code,
      fragment_code_size,
      &fragment_set_layout_count,
      fragment_set_layouts);

  uint32_t highest_set_layout_index = 0;

  for (uint32_t i = 0; i < vertex_set_layout_count; i++) {
    if (highest_set_layout_index < vertex_set_layouts[i].set_index) {
      highest_set_layout_index = vertex_set_layouts[i].set_index;
    }
  }

  for (uint32_t i = 0; i < fragment_set_layout_count; i++) {
    if (highest_set_layout_index < fragment_set_layouts[i].set_index) {
      highest_set_layout_index = fragment_set_layouts[i].set_index;
    }
  }

  *descriptor_set_layout_count = highest_set_layout_index + 1;
  *descriptor_set_layouts = (VkDescriptorSetLayout *)calloc(
      *descriptor_set_layout_count, sizeof(VkDescriptorSetLayout));

  VkDescriptorSetLayoutCreateInfo **descriptor_set_layout_create_infos =
      (VkDescriptorSetLayoutCreateInfo **)calloc(
          *descriptor_set_layout_count,
          sizeof(VkDescriptorSetLayoutCreateInfo *));

  for (uint32_t i = 0; i < vertex_set_layout_count; i++) {
    uint32_t set_index = vertex_set_layouts[i].set_index;
    if (descriptor_set_layout_create_infos[set_index] != NULL) {
      for (uint32_t j = 0;
           j < descriptor_set_layout_create_infos[set_index]->bindingCount;
           j++) {
        ((VkDescriptorSetLayoutBinding *)
             descriptor_set_layout_create_infos[set_index]
                 ->pBindings)[j]
            .stageFlags |= vertex_set_layouts[i].bindings[j].stageFlags;
      }
    } else {
      descriptor_set_layout_create_infos[set_index] =
          &vertex_set_layouts[i].create_info;
    }
  }

  for (uint32_t i = 0; i < fragment_set_layout_count; i++) {
    uint32_t set_index = fragment_set_layouts[i].set_index;
    if (descriptor_set_layout_create_infos[set_index] != NULL) {
      for (uint32_t j = 0;
           j < descriptor_set_layout_create_infos[set_index]->bindingCount;
           j++) {
        ((VkDescriptorSetLayoutBinding *)
             descriptor_set_layout_create_infos[set_index]
                 ->pBindings)[j]
            .stageFlags |= fragment_set_layouts[i].bindings[j].stageFlags;
      }
    } else {
      descriptor_set_layout_create_infos[set_index] =
          &fragment_set_layouts[i].create_info;
    }
  }

  for (uint32_t i = 0; i < *descriptor_set_layout_count; i++) {
    if (descriptor_set_layout_create_infos[i] != NULL) {
      VK_CHECK(vkCreateDescriptorSetLayout(
          g_ctx.device,
          descriptor_set_layout_create_infos[i],
          NULL,
          &(*descriptor_set_layouts)[i]));
    }
  }

  free(descriptor_set_layout_create_infos);

  for (uint32_t i_set = 0; i_set < vertex_set_layout_count; i_set++) {
    free(vertex_set_layouts[i_set].bindings);
  }
  free(vertex_set_layouts);

  for (uint32_t i_set = 0; i_set < fragment_set_layout_count; i_set++) {
    free(fragment_set_layouts[i_set].bindings);
  }
  free(fragment_set_layouts);
}

static inline void create_pipeline_layout(
    uint32_t set_layout_count,
    VkDescriptorSetLayout *set_layouts,
    VkPipelineLayout *pipeline_layout) {
  VkPushConstantRange push_constant_range = {};
  push_constant_range.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  push_constant_range.offset = 0;
  push_constant_range.size = 128;

  VkPipelineLayoutCreateInfo create_info;
  create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.setLayoutCount = set_layout_count;
  create_info.pSetLayouts = set_layouts;
  create_info.pushConstantRangeCount = 1;
  create_info.pPushConstantRanges = &push_constant_range;

  VK_CHECK(vkCreatePipelineLayout(
      g_ctx.device, &create_info, NULL, pipeline_layout));
}

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

  re_shader_init_spirv(
      shader,
      (uint32_t *)shaderc_result_get_bytes(vertex_result),
      shaderc_result_get_length(vertex_result),
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

  create_descriptor_set_layouts(
      vertex_code,
      vertex_code_size,
      fragment_code,
      fragment_code_size,
      &shader->descriptor_set_layout_count,
      &shader->descriptor_set_layouts);

  create_pipeline_layout(
      shader->descriptor_set_layout_count,
      shader->descriptor_set_layouts,
      &shader->pipeline_layout);
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

  for (uint32_t i = 0; i < shader->descriptor_set_layout_count; i++) {
    vkDestroyDescriptorSetLayout(
        g_ctx.device, shader->descriptor_set_layouts[i], NULL);
  }

  if (shader->descriptor_set_layouts != NULL) {
    free(shader->descriptor_set_layouts);
    shader->descriptor_set_layouts = NULL;
  }

  if (shader->pipeline_layout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(g_ctx.device, shader->pipeline_layout, NULL);
    shader->pipeline_layout = VK_NULL_HANDLE;
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
