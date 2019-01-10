#include "pipelines.hpp"
#include <renderer/context.hpp>
#include <renderer/util.hpp>

namespace engine {
renderer::PipelineParameters standardPipelineParameters() {
  renderer::PipelineParameters params;

  params.layout =
      renderer::ctx().m_resourceManager.m_providers.standard.pipelineLayout;

  params.rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  params.rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;

  return params;
}

renderer::PipelineParameters billboardPipelineParameters() {
  renderer::PipelineParameters params;

  params.layout =
      renderer::ctx().m_resourceManager.m_providers.billboard.pipelineLayout;

  params.vertexInputState = VkPipelineVertexInputStateCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, // sType
      nullptr,                                                   // pNext
      0,                                                         // flags
      0,       // vertexBindingDescriptionCount
      nullptr, // pVertexBindingDescriptions
      0,       // vertexAttributeDescriptionCount
      nullptr, // pVertexAttributeDescriptions
  };

  params.rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
  params.rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

  return params;
}

renderer::PipelineParameters wireframePipelineParameters() {
  renderer::PipelineParameters params;

  params.layout =
      renderer::ctx().m_resourceManager.m_providers.box.pipelineLayout;

  params.rasterizationState.cullMode = VK_CULL_MODE_NONE;
  params.rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  params.rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
  params.rasterizationState.lineWidth = 2.0f;

  return params;
}

renderer::PipelineParameters skyboxPipelineParameters() {
  renderer::PipelineParameters params;

  params.layout =
      renderer::ctx().m_resourceManager.m_providers.skybox.pipelineLayout;

  params.vertexInputState = VkPipelineVertexInputStateCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, // sType
      nullptr,                                                   // pNext
      0,                                                         // flags
      0,       // vertexBindingDescriptionCount
      nullptr, // pVertexBindingDescriptions
      0,       // vertexAttributeDescriptionCount
      nullptr, // pVertexAttributeDescriptions
  };

  params.rasterizationState.cullMode = VK_CULL_MODE_NONE;

  params.depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

  return params;
}

renderer::PipelineParameters fullscreenPipelineParameters() {
  renderer::PipelineParameters params;

  params.layout =
      renderer::ctx().m_resourceManager.m_providers.fullscreen.pipelineLayout;

  params.vertexInputState = VkPipelineVertexInputStateCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, // sType
      nullptr,                                                   // pNext
      0,                                                         // flags
      0,       // vertexBindingDescriptionCount
      nullptr, // pVertexBindingDescriptions
      0,       // vertexAttributeDescriptionCount
      nullptr, // pVertexAttributeDescriptions
  };

  params.rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
  params.rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

  params.depthStencilState.depthTestEnable = VK_FALSE;

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

  params.colorBlendState = VkPipelineColorBlendStateCreateInfo{
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
} // namespace engine
