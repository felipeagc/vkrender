#include "pipeline.h"
#include "context.h"
#include "shader.h"
#include "util.h"
#include "window.h"
#include <fstd_util.h>
#include <spirv_reflect/spirv_reflect.h>

static inline VkPipelineVertexInputStateCreateInfo
default_vertex_input_state() {
  static VkVertexInputBindingDescription vertex_binding_descriptions[] = {
      {
          0,                           // binding
          sizeof(re_vertex_t),         // stride,
          VK_VERTEX_INPUT_RATE_VERTEX, // inputRate
      },
  };

  static VkVertexInputAttributeDescription vertex_attribute_descriptions[] = {
      {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(re_vertex_t, pos)},
      {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(re_vertex_t, normal)},
      {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(re_vertex_t, uv)},
  };

  VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = ARRAY_SIZE(vertex_binding_descriptions),
      .pVertexBindingDescriptions    = vertex_binding_descriptions,
      .vertexAttributeDescriptionCount =
          ARRAY_SIZE(vertex_attribute_descriptions),
      .pVertexAttributeDescriptions = vertex_attribute_descriptions,
  };

  return vertex_input_state_create_info;
}

static inline VkPipelineInputAssemblyStateCreateInfo
default_input_assembly_state() {
  VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {
      .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE,
  };

  return input_assembly_state_create_info;
}

static inline VkPipelineViewportStateCreateInfo default_viewport_state() {
  // pViewports and pScissors are null because we're defining them through a
  // dynamic state
  VkPipelineViewportStateCreateInfo viewport_state_create_info = {
      .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .pViewports    = NULL,
      .scissorCount  = 1,
      .pScissors     = NULL,
  };

  return viewport_state_create_info;
}

static inline VkPipelineRasterizationStateCreateInfo
default_rasterization_state() {
  VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .depthClampEnable        = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode             = VK_POLYGON_MODE_FILL,
      .cullMode                = VK_CULL_MODE_BACK_BIT,
      .frontFace               = VK_FRONT_FACE_CLOCKWISE,
      .depthBiasEnable         = VK_FALSE,
      .depthBiasConstantFactor = 0.0f,
      .depthBiasClamp          = 0.0f,
      .depthBiasSlopeFactor    = 0.0f,
      .lineWidth               = 1.0f,
  };

  return rasterization_state_create_info;
}

static inline VkPipelineMultisampleStateCreateInfo
default_multisample_state(VkSampleCountFlagBits sample_count) {
  VkPhysicalDeviceFeatures device_features;
  vkGetPhysicalDeviceFeatures(g_ctx.physical_device, &device_features);
  VkBool32 has_sample_shading = device_features.sampleRateShading;

  VkBool32 sample_shading_enable =
      (VkBool32)(sample_count == VK_SAMPLE_COUNT_1_BIT);

  VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples  = sample_count,
      .sampleShadingEnable   = VK_FALSE,
      .minSampleShading      = 0.25f,
      .pSampleMask           = NULL,
      .alphaToCoverageEnable = VK_FALSE,
      .alphaToOneEnable      = VK_FALSE};

  if (has_sample_shading) {
    multisample_state_create_info.sampleShadingEnable = sample_shading_enable;
  }

  return multisample_state_create_info;
}

static inline VkPipelineDepthStencilStateCreateInfo
default_depth_stencil_state() {
  VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable       = VK_TRUE,
      .depthWriteEnable      = VK_TRUE,
      .depthCompareOp        = VK_COMPARE_OP_LESS_OR_EQUAL,
      .depthBoundsTestEnable = VK_FALSE,
      .stencilTestEnable     = VK_FALSE,
  };

  return depth_stencil_state_create_info;
}

static inline VkPipelineColorBlendStateCreateInfo default_color_blend_state() {
  static VkPipelineColorBlendAttachmentState color_blend_attachment_state = {
      .blendEnable         = VK_TRUE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .colorBlendOp        = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
      .alphaBlendOp        = VK_BLEND_OP_ADD,
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
  };

  VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {
      .sType         = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable = VK_FALSE,
      .logicOp       = VK_LOGIC_OP_COPY,
      .attachmentCount = 1,
      .pAttachments    = &color_blend_attachment_state,
      .blendConstants  = {0.0f, 0.0f, 0.0f, 0.0f},
  };

  return color_blend_state_create_info;
}

static inline VkPipelineDynamicStateCreateInfo default_dynamic_state() {
  static VkDynamicState dynamic_states[] = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
  };

  VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {
      .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = (uint32_t)ARRAY_SIZE(dynamic_states),
      .pDynamicStates    = dynamic_states,
  };

  return dynamic_state_create_info;
}

re_pipeline_parameters_t re_default_pipeline_parameters() {
  return (re_pipeline_parameters_t){
      .vertex_input_state   = default_vertex_input_state(),
      .input_assembly_state = default_input_assembly_state(),
      .viewport_state       = default_viewport_state(),
      .rasterization_state  = default_rasterization_state(),
      .depth_stencil_state  = default_depth_stencil_state(),
      .color_blend_state    = default_color_blend_state(),
      .dynamic_state        = default_dynamic_state(),
  };
}

void re_pipeline_layout_init(
    re_pipeline_layout_t *layout,
    const re_shader_t *shaders,
    uint32_t shader_count) {
  memset(layout, 0, sizeof(*layout));
  assert(shader_count <= RE_MAX_SHADER_STAGES);

  SpvReflectShaderModule modules[RE_MAX_SHADER_STAGES] = {0};

  SpvReflectResult result;

  for (uint32_t i = 0; i < shader_count; i++) {
    result = spvReflectCreateShaderModule(
        shaders[i].code_size, shaders[i].code, &modules[i]);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
  }

  for (uint32_t i = 0; i < shader_count; i++) {
    SpvReflectShaderModule *mod = &modules[i];

    layout->stage_flags[i] = (VkShaderStageFlagBits)mod->shader_stage;

    for (uint32_t s = 0; s < mod->descriptor_set_count; s++) {
      SpvReflectDescriptorSet *set = &mod->descriptor_sets[s];
      layout->descriptor_set_count =
          MAX(layout->descriptor_set_count, set->set + 1);
    }
  }

  assert(layout->descriptor_set_count <= RE_MAX_DESCRIPTOR_SETS);

  uint32_t binding_counts[RE_MAX_DESCRIPTOR_SETS] = {0};

  re_descriptor_set_layout_t alloc_layouts[RE_MAX_DESCRIPTOR_SETS] = {0};

  for (uint32_t i = 0; i < shader_count; i++) {
    SpvReflectShaderModule *mod = &modules[i];

    for (uint32_t s = 0; s < mod->descriptor_set_count; s++) {
      SpvReflectDescriptorSet *set = &mod->descriptor_sets[s];

      binding_counts[set->set] = set->binding_count;

      for (uint32_t b = 0; b < set->binding_count; b++) {
        SpvReflectDescriptorBinding *binding = set->bindings[b];
        VkDescriptorType desc_type = (VkDescriptorType)binding->descriptor_type;

        if (desc_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
          desc_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        }

        alloc_layouts[set->set].array_size[binding->binding] =
            MAX(alloc_layouts[set->set].array_size[binding->binding],
                binding->count);
        alloc_layouts[set->set].stage_flags[binding->binding] |=
            mod->shader_stage;

        if (desc_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
          alloc_layouts[set->set].uniform_buffer_mask |= 1u << binding->binding;
        }

        if (desc_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {
          alloc_layouts[set->set].uniform_buffer_dynamic_mask |=
              1u << binding->binding;
        }

        if (binding->descriptor_type ==
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
          alloc_layouts[set->set].combined_image_sampler_mask |=
              1u << binding->binding;
        }
      }
    }
  }

  VkDescriptorSetLayout set_layouts[RE_MAX_DESCRIPTOR_SETS] = {0};

  // Request the allocator
  for (uint32_t i = 0; i < layout->descriptor_set_count; i++) {
    layout->descriptor_set_allocators[i] =
        rx_ctx_request_descriptor_set_allocator(alloc_layouts[i]);
    set_layouts[i] = layout->descriptor_set_allocators[i]->set_layout;
  }

  memset(layout->push_constants, 0, sizeof(layout->push_constants));
  layout->push_constant_count = 0;

  for (uint32_t i = 0; i < shader_count; i++) {
    SpvReflectShaderModule *mod = &modules[i];

    layout->push_constant_count =
        MAX(layout->push_constant_count, mod->push_constant_block_count);

    for (uint32_t p = 0; p < mod->push_constant_block_count; p++) {
      SpvReflectBlockVariable *pc = &mod->push_constant_blocks[p];

      layout->push_constants[p].stageFlags |= mod->shader_stage;
      layout->push_constants[p].offset = pc->absolute_offset;
      layout->push_constants[p].size =
          MAX(layout->push_constants[p].size, pc->size);
    }
  }

  VK_CHECK(vkCreatePipelineLayout(
      g_ctx.device,
      &(VkPipelineLayoutCreateInfo){
          .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
          .setLayoutCount = layout->descriptor_set_count,
          .pSetLayouts    = set_layouts,
          .pushConstantRangeCount = layout->push_constant_count,
          .pPushConstantRanges    = layout->push_constants,
      },
      NULL,
      &layout->layout));

  for (uint32_t i = 0; i < shader_count; i++) {
    spvReflectDestroyShaderModule(&modules[i]);
  }
}

void re_pipeline_layout_destroy(re_pipeline_layout_t *layout) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  if (layout->layout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(g_ctx.device, layout->layout, NULL);
  }
}

void re_pipeline_init_graphics(
    re_pipeline_t *pipeline,
    const re_render_target_t *render_target,
    const re_shader_t *shaders,
    uint32_t shader_count,
    const re_pipeline_parameters_t parameters) {
  assert(shader_count <= RE_MAX_SHADER_STAGES);

  pipeline->bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;

  re_pipeline_layout_init(&pipeline->layout, shaders, shader_count);

  VkPipelineShaderStageCreateInfo pipeline_stages[RE_MAX_SHADER_STAGES];

  for (uint32_t i = 0; i < shader_count; i++) {
    pipeline_stages[i] = (VkPipelineShaderStageCreateInfo){
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = pipeline->layout.stage_flags[i],
        .module = shaders[i].module,
        .pName  = "main",
        .pSpecializationInfo = NULL,
    };
  }

  VkPipelineMultisampleStateCreateInfo multisample_state =
      default_multisample_state(render_target->sample_count);

  VkGraphicsPipelineCreateInfo pipeline_create_info = {
      .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount          = shader_count,
      .pStages             = pipeline_stages,
      .pVertexInputState   = &parameters.vertex_input_state,
      .pInputAssemblyState = &parameters.input_assembly_state,
      .pTessellationState  = NULL,
      .pViewportState      = &parameters.viewport_state,
      .pRasterizationState = &parameters.rasterization_state,
      .pMultisampleState   = &multisample_state,
      .pDepthStencilState  = &parameters.depth_stencil_state,
      .pColorBlendState    = &parameters.color_blend_state,
      .pDynamicState       = &parameters.dynamic_state,
      .layout              = pipeline->layout.layout,
      .renderPass          = render_target->render_pass,
      .subpass             = 0,
      .basePipelineHandle  = 0,
      .basePipelineIndex   = -1,
  };

  VK_CHECK(vkCreateGraphicsPipelines(
      g_ctx.device,
      VK_NULL_HANDLE,
      1,
      &pipeline_create_info,
      NULL,
      &pipeline->pipeline));
}

void re_pipeline_destroy(re_pipeline_t *pipeline) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  re_pipeline_layout_destroy(&pipeline->layout);

  if (pipeline->pipeline != VK_NULL_HANDLE) {
    vkDestroyPipeline(g_ctx.device, pipeline->pipeline, NULL);
  }
}
