#include "pipeline.h"
#include "context.h"
#include "shader.h"
#include "util.h"
#include "window.h"
#include <fstd_util.h>

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
      ARRAYSIZE(vertex_binding_descriptions), // vertexBindingDescriptionCount
      vertex_binding_descriptions,            // pVertexBindingDescriptions
      ARRAYSIZE(
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
  VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {};
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
      0,                                   // flags
      (uint32_t)ARRAYSIZE(dynamic_states), // dynamicStateCount
      dynamic_states,                      // pDyanmicStates
  };

  return dynamic_state_create_info;
}

re_pipeline_parameters_t re_default_pipeline_parameters() {
  re_pipeline_parameters_t params;
  params.vertex_input_state = default_vertex_input_state();
  params.input_assembly_state = default_input_assembly_state();
  params.viewport_state = default_viewport_state();
  params.rasterization_state = default_rasterization_state();
  params.depth_stencil_state = default_depth_stencil_state();
  params.color_blend_state = default_color_blend_state();
  params.dynamic_state = default_dynamic_state();
  params.pipeline_layout = VK_NULL_HANDLE;
  return params;
}

void re_pipeline_init_graphics(
    re_pipeline_t *pipeline,
    const re_render_target_t render_target,
    const re_shader_t *shader,
    const re_pipeline_parameters_t parameters) {
  pipeline->layout = parameters.pipeline_layout;

  VkPipelineShaderStageCreateInfo pipeline_stages[2];
  re_shader_get_pipeline_stages(shader, pipeline_stages);

  VkPipelineMultisampleStateCreateInfo multisample_state =
      default_multisample_state(render_target.sample_count);

  VkGraphicsPipelineCreateInfo pipeline_create_info = {
      VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      NULL,
      0,                                // flags
      ARRAYSIZE(pipeline_stages),       // stageCount
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
      pipeline->layout,                 // pipelineLayout
      render_target.render_pass,        // renderPass
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

void re_pipeline_bind_graphics(
    re_pipeline_t *pipeline, struct re_window_t *window) {
  VkCommandBuffer command_buffer = re_window_get_current_command_buffer(window);
  vkCmdBindPipeline(
      command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
}

void re_pipeline_destroy(re_pipeline_t *pipeline) {
  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  if (pipeline->pipeline != VK_NULL_HANDLE) {
    vkDestroyPipeline(g_ctx.device, pipeline->pipeline, NULL);
  }
}
