#include "pipeline.hpp"
#include "context.hpp"
#include "shader.hpp"
#include "util.hpp"
#include "window.hpp"

namespace vkr {

void GraphicsPipeline::destroy() {
  vkDestroyPipelineLayout(ctx::device, this->pipelineLayout, nullptr);
  vkDestroyPipeline(ctx::device, this->pipeline, nullptr);
}

GraphicsPipeline createStandardPipeline(Window &window, Shader &shader) {
  VkDescriptorSetLayout descriptorSetLayouts[] = {
      *ctx::descriptorManager.getSetLayout(vkr::DESC_CAMERA),
      *ctx::descriptorManager.getSetLayout(vkr::DESC_MATERIAL),
      *ctx::descriptorManager.getSetLayout(vkr::DESC_MESH),
      *ctx::descriptorManager.getSetLayout(vkr::DESC_LIGHTING),
  };

  auto pipelineLayout = pipeline::createPipelineLayout(
      ARRAYSIZE(descriptorSetLayouts), descriptorSetLayouts);

  auto shaderStageCreateInfos = shader.getPipelineShaderStageCreateInfos();

  auto vertexInputStateCreateInfo = pipeline::defaultVertexInputState();
  auto inputAssemblyStateCreateInfo = pipeline::defaultInputAssemblyState();
  auto viewportStateCreateInfo = pipeline::defaultViewportState();
  auto rasterizationStateCreateInfo = pipeline::defaultRasterizationState();
  auto multisampleStateCreateInfo =
      pipeline::defaultMultisampleState(window.getMSAASamples());
  auto depthStencilStateCreateInfo = pipeline::defaultDepthStencilState();
  auto colorBlendStateCreateInfo = pipeline::defaultColorBlendState();
  auto dynamicStateCreateInfo = pipeline::defaultDynamicState();

  VkGraphicsPipelineCreateInfo pipelineCreateInfo{
      VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      nullptr,
      0,                                                    // flags
      static_cast<uint32_t>(shaderStageCreateInfos.size()), // stageCount
      shaderStageCreateInfos.data(),                        // pStages
      &vertexInputStateCreateInfo,                          // pVertexInputState
      &inputAssemblyStateCreateInfo, // pInputAssemblyState
      nullptr,                       // pTesselationState
      &viewportStateCreateInfo,      // pViewportState
      &rasterizationStateCreateInfo, // pRasterizationState
      &multisampleStateCreateInfo,   // multisampleState
      &depthStencilStateCreateInfo,  // pDepthStencilState
      &colorBlendStateCreateInfo,    // pColorBlendState
      &dynamicStateCreateInfo,       // pDynamicState
      pipelineLayout,                // pipelineLayout
      window.getRenderPass(),        // renderPass
      0,                             // subpass
      {},                            // basePipelineHandle
      -1                             // basePipelineIndex
  };

  VkPipeline pipeline;

  VK_CHECK(vkCreateGraphicsPipelines(
      ctx::device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline));

  return GraphicsPipeline{pipeline, pipelineLayout};
}
} // namespace vkr

namespace vkr::pipeline {
VkPipelineLayout createPipelineLayout(
    uint32_t setLayoutCount, VkDescriptorSetLayout *descriptorSetLayouts) {
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      nullptr,
      0,
      setLayoutCount,       // setLayoutCount
      descriptorSetLayouts, // pSetLayouts
      0,                    // pushConstantRangeCount
      nullptr,              // pPushConstantRanges
  };

  VkPipelineLayout pipelineLayout;

  VK_CHECK(vkCreatePipelineLayout(
      ctx::device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

  return pipelineLayout;
}

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
      0,                               // flags
      VK_FALSE,                        // depthClampEnable
      VK_FALSE,                        // rasterizerDiscardEnable
      VK_POLYGON_MODE_FILL,            // polygonMode
      VK_CULL_MODE_FRONT_BIT,          // cullMode
      VK_FRONT_FACE_COUNTER_CLOCKWISE, // frontFace
      VK_FALSE,                        // depthBiasEnable
      0.0f,                            // depthBiasConstantFactor,
      0.0f,                            // depthBiasClamp
      0.0f,                            // depthBiasSlopeFactor
      1.0f,                            // lineWidth
  };

  return rasterizationStateCreateInfo;
}

VkPipelineMultisampleStateCreateInfo
defaultMultisampleState(VkSampleCountFlagBits sampleCount) {
  VkPhysicalDeviceFeatures deviceFeatures;
  vkGetPhysicalDeviceFeatures(ctx::physicalDevice, &deviceFeatures);
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
} // namespace vkr::pipeline
