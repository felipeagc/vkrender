#include "pipelines.hpp"
#include <renderer/context.hpp>
#include <renderer/util.hpp>

re_pipeline_parameters_t eg_standard_pipeline_parameters() {
  re_pipeline_parameters_t params = re_default_pipeline_parameters();

  params.layout = g_ctx.resource_manager.providers.standard.pipeline_layout;

  params.rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  params.rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;

  return params;
}

re_pipeline_parameters_t eg_billboard_pipeline_parameters() {
  re_pipeline_parameters_t params = re_default_pipeline_parameters();

  params.layout = g_ctx.resource_manager.providers.billboard.pipeline_layout;

  params.vertex_input_state = VkPipelineVertexInputStateCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, // sType
      nullptr,                                                   // pNext
      0,                                                         // flags
      0,       // vertexBindingDescriptionCount
      nullptr, // pVertexBindingDescriptions
      0,       // vertexAttributeDescriptionCount
      nullptr, // pVertexAttributeDescriptions
  };

  params.rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;
  params.rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

  return params;
}

re_pipeline_parameters_t eg_wireframe_pipeline_parameters() {
  re_pipeline_parameters_t params = re_default_pipeline_parameters();

  params.layout = g_ctx.resource_manager.providers.wireframe.pipeline_layout;

  params.rasterization_state.cullMode = VK_CULL_MODE_NONE;
  params.rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  params.rasterization_state.polygonMode = VK_POLYGON_MODE_LINE;
  params.rasterization_state.lineWidth = 2.0f;

  return params;
}

re_pipeline_parameters_t eg_skybox_pipeline_parameters() {
  re_pipeline_parameters_t params = re_default_pipeline_parameters();

  params.layout = g_ctx.resource_manager.providers.skybox.pipeline_layout;

  params.vertex_input_state = VkPipelineVertexInputStateCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, // sType
      nullptr,                                                   // pNext
      0,                                                         // flags
      0,       // vertexBindingDescriptionCount
      nullptr, // pVertexBindingDescriptions
      0,       // vertexAttributeDescriptionCount
      nullptr, // pVertexAttributeDescriptions
  };

  params.rasterization_state.cullMode = VK_CULL_MODE_NONE;

  params.depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

  return params;
}

re_pipeline_parameters_t eg_fullscreen_pipeline_parameters() {
  re_pipeline_parameters_t params = re_default_pipeline_parameters();

  params.layout = g_ctx.resource_manager.providers.fullscreen.pipeline_layout;

  params.vertex_input_state = VkPipelineVertexInputStateCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, // sType
      nullptr,                                                   // pNext
      0,                                                         // flags
      0,       // vertexBindingDescriptionCount
      nullptr, // pVertexBindingDescriptions
      0,       // vertexAttributeDescriptionCount
      nullptr, // pVertexAttributeDescriptions
  };

  params.rasterization_state.cullMode = VK_CULL_MODE_FRONT_BIT;
  params.rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

  params.depth_stencil_state.depthTestEnable = VK_FALSE;

  static VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {
      VK_FALSE,                            // blendEnable
      VK_BLEND_FACTOR_SRC_ALPHA,           // srcColorBlendFactor
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // dstColorBlendFactor
      VK_BLEND_OP_ADD,                     // colorBlendOp
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // srcAlphaBlendFactor
      VK_BLEND_FACTOR_ZERO,                // dstAlphaBlendFactor
      VK_BLEND_OP_ADD,                     // alphaBlendOp
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, // colorWriteMask
  };

  params.color_blend_state = VkPipelineColorBlendStateCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      nullptr,
      0,                          // flags
      VK_FALSE,                   // logicOpEnable
      VK_LOGIC_OP_COPY,           // logicOp
      1,                          // attachmentCount
      &colorBlendAttachmentState, // pAttachments
      {0.0f, 0.0f, 0.0f, 0.0f},   // blendConstants
  };

  return params;
}
