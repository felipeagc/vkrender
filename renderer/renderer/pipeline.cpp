#include "pipeline.hpp"
#include "context.hpp"
#include "shader.hpp"
#include "util.hpp"

namespace renderer {

GraphicsPipeline::GraphicsPipeline(
    VkPipeline pipeline, VkPipelineLayout pipelineLayout)
    : pipeline(pipeline), layout(pipelineLayout) {}

GraphicsPipeline::GraphicsPipeline(
    const RenderTarget &renderTarget,
    const re_shader_t shader,
    const PipelineParameters &parameters) {
  layout = parameters.layout;

  VkPipelineShaderStageCreateInfo pipeline_stages[2];
  re_shader_get_pipeline_stages(&shader, pipeline_stages);

  auto multisampleState =
      pipeline::defaultMultisampleState(renderTarget.getSampleCount());

  VkGraphicsPipelineCreateInfo pipelineCreateInfo{
      VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      nullptr,
      0,                              // flags
      ARRAYSIZE(pipeline_stages),     // stageCount
      pipeline_stages,                // pStages
      &parameters.vertexInputState,   // pVertexInputState
      &parameters.inputAssemblyState, // pInputAssemblyState
      (parameters.hasTesselationState ? &parameters.tessellationState
                                      : nullptr), // pTesselationState
      &parameters.viewportState,                  // pViewportState
      &parameters.rasterizationState,             // pRasterizationState
      &multisampleState,                          // multisampleState
      &parameters.depthStencilState,              // pDepthStencilState
      &parameters.colorBlendState,                // pColorBlendState
      &parameters.dynamicState,                   // pDynamicState
      layout,                                     // pipelineLayout
      renderTarget.getRenderPass(),               // renderPass
      0,                                          // subpass
      {},                                         // basePipelineHandle
      -1                                          // basePipelineIndex
  };

  VK_CHECK(vkCreateGraphicsPipelines(
      ctx().m_device,
      VK_NULL_HANDLE,
      1,
      &pipelineCreateInfo,
      nullptr,
      &this->pipeline));
}

GraphicsPipeline::~GraphicsPipeline() {
  VK_CHECK(vkDeviceWaitIdle(ctx().m_device));

  if (this->pipeline != VK_NULL_HANDLE) {
    vkDestroyPipeline(ctx().m_device, this->pipeline, nullptr);
  }
}

GraphicsPipeline::GraphicsPipeline(GraphicsPipeline &&rhs) {
  this->pipeline = rhs.pipeline;
  layout = rhs.layout;
  rhs.pipeline = VK_NULL_HANDLE;
  rhs.layout = VK_NULL_HANDLE;
}

GraphicsPipeline &GraphicsPipeline::operator=(GraphicsPipeline &&rhs) {
  VK_CHECK(vkDeviceWaitIdle(ctx().m_device));

  if (this->pipeline != VK_NULL_HANDLE) {
    vkDestroyPipeline(ctx().m_device, this->pipeline, nullptr);
  }

  this->pipeline = rhs.pipeline;
  layout = rhs.layout;
  rhs.pipeline = VK_NULL_HANDLE;
  rhs.layout = VK_NULL_HANDLE;

  return *this;
}
} // namespace renderer

namespace renderer::pipeline {
VkPipelineVertexInputStateCreateInfo defaultVertexInputState() {
  static VkVertexInputBindingDescription vertexBindingDescriptions[] = {
      {
          0,                           // binding
          sizeof(StandardVertex),      // stride,
          VK_VERTEX_INPUT_RATE_VERTEX, // inputRate
      },
  };

  static VkVertexInputAttributeDescription vertexAttributeDescriptions[] = {
      {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(StandardVertex, pos)},
      {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(StandardVertex, normal)},
      {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(StandardVertex, uv)},
  };

  VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, // sType
      nullptr,                                                   // pNext
      0,                                                         // flags
      ARRAYSIZE(vertexBindingDescriptions),   // vertexBindingDescriptionCount
      vertexBindingDescriptions,              // pVertexBindingDescriptions
      ARRAYSIZE(vertexAttributeDescriptions), // vertexAttributeDescriptionCount
      vertexAttributeDescriptions,            // pVertexAttributeDescriptions
  };

  return vertexInputStateCreateInfo;
}

VkPipelineInputAssemblyStateCreateInfo defaultInputAssemblyState() {
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      nullptr,
      0,                                   // flags
      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, // topology
      VK_FALSE                             // primitiveRestartEnable
  };

  return inputAssemblyStateCreateInfo;
}

VkPipelineViewportStateCreateInfo defaultViewportState() {
  // pViewports and pScissors are null because we're defining them through a
  // dynamic state
  VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      nullptr,
      0,       // flags
      1,       // viewportCount
      nullptr, // pViewports
      1,       // scissorCount
      nullptr  // pScissors
  };

  return viewportStateCreateInfo;
}

VkPipelineRasterizationStateCreateInfo defaultRasterizationState() {
  VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      nullptr,
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

  return rasterizationStateCreateInfo;
}

VkPipelineMultisampleStateCreateInfo
defaultMultisampleState(VkSampleCountFlagBits sampleCount) {
  VkPhysicalDeviceFeatures deviceFeatures;
  vkGetPhysicalDeviceFeatures(ctx().m_physicalDevice, &deviceFeatures);
  VkBool32 hasSampleShading = deviceFeatures.sampleRateShading;

  VkBool32 sampleShadingEnable =
      (sampleCount == VK_SAMPLE_COUNT_1_BIT ? VK_FALSE : VK_TRUE);

  VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      nullptr,
      0,           // flags
      sampleCount, // rasterizationSamples
      VK_FALSE,    // sampleShadingEnable
      0.25f,       // minSampleShading
      nullptr,     // pSampleMask
      VK_FALSE,    // alphaToCoverageEnable
      VK_FALSE     // alphaToOneEnable
  };

  if (hasSampleShading) {
    multisampleStateCreateInfo.sampleShadingEnable = sampleShadingEnable;
  }

  return multisampleStateCreateInfo;
}

VkPipelineDepthStencilStateCreateInfo defaultDepthStencilState() {
  VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
  depthStencilStateCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencilStateCreateInfo.pNext = nullptr;
  depthStencilStateCreateInfo.flags = 0;
  depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
  depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
  depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
  depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;

  return depthStencilStateCreateInfo;
}

VkPipelineColorBlendStateCreateInfo defaultColorBlendState() {
  static VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {
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

  VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      nullptr,
      0,                          // flags
      VK_FALSE,                   // logicOpEnable
      VK_LOGIC_OP_COPY,           // logicOp
      1,                          // attachmentCount
      &colorBlendAttachmentState, // pAttachments
      {0.0f, 0.0f, 0.0f, 0.0f},   // blendConstants
  };

  return colorBlendStateCreateInfo;
}

VkPipelineDynamicStateCreateInfo defaultDynamicState() {
  static VkDynamicState dynamicStates[] = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
  };

  VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      nullptr,
      0,                                               // flags
      static_cast<uint32_t>(ARRAYSIZE(dynamicStates)), // dynamicStateCount
      dynamicStates,                                   // pDyanmicStates
  };

  return dynamicStateCreateInfo;
}
} // namespace renderer::pipeline
