#include "pipeline.h"
#include "context.h"
#include "shader.h"
#include "util.h"
#include "window.h"
#include <fstd_util.h>
#include <spirv_reflect.h>

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
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, // sType
      NULL,                                                      // pNext
      0,                                                         // flags
      ARRAY_SIZE(vertex_binding_descriptions), // vertexBindingDescriptionCount
      vertex_binding_descriptions,             // pVertexBindingDescriptions
      ARRAY_SIZE(
          vertex_attribute_descriptions), // vertexAttributeDescriptionCount
      vertex_attribute_descriptions,      // pVertexAttributeDescriptions
  };

  return vertex_input_state_create_info;
}

static inline VkPipelineInputAssemblyStateCreateInfo
default_input_assembly_state() {
  VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      NULL,
      0,                                   // flags
      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, // topology
      VK_FALSE                             // primitiveRestartEnable
  };

  return input_assembly_state_create_info;
}

static inline VkPipelineViewportStateCreateInfo default_viewport_state() {
  // pViewports and pScissors are null because we're defining them through a
  // dynamic state
  VkPipelineViewportStateCreateInfo viewport_state_create_info = {
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      NULL,
      0,    // flags
      1,    // viewportCount
      NULL, // pViewports
      1,    // scissorCount
      NULL  // pScissors
  };

  return viewport_state_create_info;
}

static inline VkPipelineRasterizationStateCreateInfo
default_rasterization_state() {
  VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      NULL,
      0,                       // flags
      VK_FALSE,                // depthClampEnable
      VK_FALSE,                // rasterizerDiscardEnable
      VK_POLYGON_MODE_FILL,    // polygonMode
      VK_CULL_MODE_BACK_BIT,   // cullMode
      VK_FRONT_FACE_CLOCKWISE, // frontFace
      VK_FALSE,                // depthBiasEnable
      0.0f,                    // depthBiasConstantFactor,
      0.0f,                    // depthBiasClamp
      0.0f,                    // depthBiasSlopeFactor
      1.0f,                    // lineWidth
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
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      NULL,
      0,            // flags
      sample_count, // rasterizationSamples
      VK_FALSE,     // sampleShadingEnable
      0.25f,        // minSampleShading
      NULL,         // pSampleMask
      VK_FALSE,     // alphaToCoverageEnable
      VK_FALSE      // alphaToOneEnable
  };

  if (has_sample_shading) {
    multisample_state_create_info.sampleShadingEnable = sample_shading_enable;
  }

  return multisample_state_create_info;
}

static inline VkPipelineDepthStencilStateCreateInfo
default_depth_stencil_state() {
  VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {0};
  depth_stencil_state_create_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depth_stencil_state_create_info.pNext = NULL;
  depth_stencil_state_create_info.flags = 0;
  depth_stencil_state_create_info.depthTestEnable = VK_TRUE;
  depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
  depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
  depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;

  return depth_stencil_state_create_info;
}

static inline VkPipelineColorBlendStateCreateInfo default_color_blend_state() {
  static VkPipelineColorBlendAttachmentState color_blend_attachment_state = {
      VK_TRUE,                             // blendEnable
      VK_BLEND_FACTOR_SRC_ALPHA,           // srcColorBlendFactor
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // dstColorBlendFactor
      VK_BLEND_OP_ADD,                     // colorBlendOp
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // srcAlphaBlendFactor
      VK_BLEND_FACTOR_ZERO,                // dstAlphaBlendFactor
      VK_BLEND_OP_ADD,                     // alphaBlendOp
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, // colorWriteMask
  };

  VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      NULL,
      0,                             // flags
      VK_FALSE,                      // logicOpEnable
      VK_LOGIC_OP_COPY,              // logicOp
      1,                             // attachmentCount
      &color_blend_attachment_state, // pAttachments
      {0.0f, 0.0f, 0.0f, 0.0f},      // blendConstants
  };

  return color_blend_state_create_info;
}

static inline VkPipelineDynamicStateCreateInfo default_dynamic_state() {
  static VkDynamicState dynamic_states[] = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
  };

  VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {
      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      NULL,
      0,                                    // flags
      (uint32_t)ARRAY_SIZE(dynamic_states), // dynamicStateCount
      dynamic_states,                       // pDyanmicStates
  };

  return dynamic_state_create_info;
}

re_pipeline_parameters_t re_default_pipeline_parameters() {
  re_pipeline_parameters_t params = {0};
  params.vertex_input_state = default_vertex_input_state();
  params.input_assembly_state = default_input_assembly_state();
  params.viewport_state = default_viewport_state();
  params.rasterization_state = default_rasterization_state();
  params.depth_stencil_state = default_depth_stencil_state();
  params.color_blend_state = default_color_blend_state();
  params.dynamic_state = default_dynamic_state();
  return params;
}

void re_pipeline_layout_init(
    re_pipeline_layout_t *layout,
    const re_shader_t *vertex_shader,
    const re_shader_t *fragment_shader) {
  memset(layout, 0, sizeof(*layout));

  SpvReflectShaderModule modules[2] = {0};

  SpvReflectResult result;
  result = spvReflectCreateShaderModule(
      vertex_shader->code_size, vertex_shader->code, &modules[0]);
  assert(result == SPV_REFLECT_RESULT_SUCCESS);
  result = spvReflectCreateShaderModule(
      fragment_shader->code_size, fragment_shader->code, &modules[1]);
  assert(result == SPV_REFLECT_RESULT_SUCCESS);

  uint32_t set_count = 0;

  for (uint32_t i = 0; i < ARRAY_SIZE(modules); i++) {
    SpvReflectShaderModule *mod = &modules[i];

    for (uint32_t s = 0; s < mod->descriptor_set_count; s++) {
      SpvReflectDescriptorSet *set = &mod->descriptor_sets[s];
      set_count = MAX(set_count, set->set + 1);
    }
  }

  assert(set_count <= RE_MAX_DESCRIPTOR_SETS);

  uint32_t binding_counts[RE_MAX_DESCRIPTOR_SETS] = {0};

  re_descriptor_set_layout_t alloc_layouts[RE_MAX_DESCRIPTOR_SETS] = {0};

  for (uint32_t i = 0; i < ARRAY_SIZE(modules); i++) {
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
  for (uint32_t i = 0; i < set_count; i++) {
    layout->descriptor_set_allocators[i] =
        rx_ctx_request_descriptor_set_allocator(alloc_layouts[i]);
    set_layouts[i] = layout->descriptor_set_allocators[i]->set_layout;
  }

  memset(layout->push_constants, 0, sizeof(layout->push_constants));
  layout->push_constant_count = 0;

  for (uint32_t i = 0; i < ARRAY_SIZE(modules); i++) {
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
          .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
          .pNext = NULL,
          .flags = 0,
          .setLayoutCount = set_count,
          .pSetLayouts = set_layouts,
          .pushConstantRangeCount = layout->push_constant_count,
          .pPushConstantRanges = layout->push_constants,
      },
      NULL,
      &layout->layout));

  for (uint32_t i = 0; i < ARRAY_SIZE(modules); i++) {
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
    const re_shader_t *vertex_shader,
    const re_shader_t *fragment_shader,
    const re_pipeline_parameters_t parameters) {
  pipeline->bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;

  re_pipeline_layout_init(&pipeline->layout, vertex_shader, fragment_shader);

  VkPipelineShaderStageCreateInfo pipeline_stages[] = {
      (VkPipelineShaderStageCreateInfo){
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .pNext = NULL,
          .flags = 0,
          .stage = VK_SHADER_STAGE_VERTEX_BIT,
          .module = vertex_shader->module,
          .pName = "main",
          .pSpecializationInfo = NULL,
      },
      (VkPipelineShaderStageCreateInfo){
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .pNext = NULL,
          .flags = 0,
          .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
          .module = fragment_shader->module,
          .pName = "main",
          .pSpecializationInfo = NULL,
      },
  };

  VkPipelineMultisampleStateCreateInfo multisample_state =
      default_multisample_state(render_target->sample_count);

  VkGraphicsPipelineCreateInfo pipeline_create_info = {
      VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      NULL,
      0,                                // flags
      ARRAY_SIZE(pipeline_stages),      // stageCount
      pipeline_stages,                  // pStages
      &parameters.vertex_input_state,   // pVertexInputState
      &parameters.input_assembly_state, // pInputAssemblyState
      NULL,                             // pTesselationState
      &parameters.viewport_state,       // pViewportState
      &parameters.rasterization_state,  // pRasterizationState
      &multisample_state,               // multisampleState
      &parameters.depth_stencil_state,  // pDepthStencilState
      &parameters.color_blend_state,    // pColorBlendState
      &parameters.dynamic_state,        // pDynamicState
      pipeline->layout.layout,          // pipelineLayout
      render_target->render_pass,       // renderPass
      0,                                // subpass
      0,                                // basePipelineHandle
      -1                                // basePipelineIndex
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
