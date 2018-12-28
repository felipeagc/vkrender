#include "pipeline.hpp"
#include "context.hpp"
#include "shader.hpp"
#include "util.hpp"
#include "window.hpp"

namespace renderer {

GraphicsPipeline::GraphicsPipeline(
    VkPipeline pipeline, VkPipelineLayout pipelineLayout)
    : m_pipeline(pipeline), m_pipelineLayout(pipelineLayout) {}

GraphicsPipeline::~GraphicsPipeline() {
  VK_CHECK(vkDeviceWaitIdle(ctx().m_device));

  if (m_pipelineLayout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(ctx().m_device, m_pipelineLayout, nullptr);
  }
  if (m_pipeline != VK_NULL_HANDLE) {
    vkDestroyPipeline(ctx().m_device, m_pipeline, nullptr);
  }
}

GraphicsPipeline::GraphicsPipeline(GraphicsPipeline &&rhs) {
  m_pipeline = rhs.m_pipeline;
  m_pipelineLayout = rhs.m_pipelineLayout;
  rhs.m_pipeline = VK_NULL_HANDLE;
  rhs.m_pipelineLayout = VK_NULL_HANDLE;
}

GraphicsPipeline &GraphicsPipeline::operator=(GraphicsPipeline &&rhs) {
  VK_CHECK(vkDeviceWaitIdle(ctx().m_device));

  if (m_pipelineLayout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(ctx().m_device, m_pipelineLayout, nullptr);
  }
  if (m_pipeline != VK_NULL_HANDLE) {
    vkDestroyPipeline(ctx().m_device, m_pipeline, nullptr);
  }

  m_pipeline = rhs.m_pipeline;
  m_pipelineLayout = rhs.m_pipelineLayout;
  rhs.m_pipeline = VK_NULL_HANDLE;
  rhs.m_pipelineLayout = VK_NULL_HANDLE;

  return *this;
}

StandardPipeline::StandardPipeline(Window &window, Shader &shader) {
  VkDescriptorSetLayout descriptorSetLayouts[] = {
      *ctx().m_descriptorManager.getSetLayout(renderer::DESC_CAMERA),
      *ctx().m_descriptorManager.getSetLayout(renderer::DESC_MATERIAL),
      *ctx().m_descriptorManager.getSetLayout(renderer::DESC_MESH),
      *ctx().m_descriptorManager.getSetLayout(renderer::DESC_MODEL),
      *ctx().m_descriptorManager.getSetLayout(renderer::DESC_ENVIRONMENT),
  };

  VkPushConstantRange pushConstantRange = {};
  pushConstantRange.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = 128;

  // Create pipeline layout
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      nullptr,
      0,
      ARRAYSIZE(descriptorSetLayouts), // setLayoutCount
      descriptorSetLayouts,            // pSetLayouts
      1,                               // pushConstantRangeCount
      &pushConstantRange,              // pPushConstantRanges
  };

  VK_CHECK(vkCreatePipelineLayout(
      ctx().m_device, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout));

  auto shaderStageCreateInfos = shader.getPipelineShaderStageCreateInfos();

  auto vertexInputStateCreateInfo = pipeline::defaultVertexInputState();
  auto inputAssemblyStateCreateInfo = pipeline::defaultInputAssemblyState();
  auto viewportStateCreateInfo = pipeline::defaultViewportState();
  auto rasterizationStateCreateInfo = pipeline::defaultRasterizationState();
  rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
  auto multisampleStateCreateInfo =
      pipeline::defaultMultisampleState(window.getSampleCount());
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
      m_pipelineLayout,              // pipelineLayout
      window.getRenderPass(),        // renderPass
      0,                             // subpass
      {},                            // basePipelineHandle
      -1                             // basePipelineIndex
  };

  VK_CHECK(vkCreateGraphicsPipelines(
      ctx().m_device,
      VK_NULL_HANDLE,
      1,
      &pipelineCreateInfo,
      nullptr,
      &m_pipeline));
}

BillboardPipeline::BillboardPipeline(Window &window, Shader &shader) {
  VkDescriptorSetLayout descriptorSetLayouts[] = {
      *ctx().m_descriptorManager.getSetLayout(renderer::DESC_CAMERA),
      *ctx().m_descriptorManager.getSetLayout(renderer::DESC_MATERIAL),
      *ctx().m_descriptorManager.getSetLayout(renderer::DESC_MODEL),
  };

  VkPushConstantRange pushConstantRange = {};
  pushConstantRange.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = 128;

  // Create pipeline layout
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      nullptr,
      0,
      ARRAYSIZE(descriptorSetLayouts), // setLayoutCount
      descriptorSetLayouts,            // pSetLayouts
      1,                               // pushConstantRangeCount
      &pushConstantRange,              // pPushConstantRanges
  };

  VK_CHECK(vkCreatePipelineLayout(
      ctx().m_device, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout));

  auto shaderStageCreateInfos = shader.getPipelineShaderStageCreateInfos();

  VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, // sType
      nullptr,                                                   // pNext
      0,                                                         // flags
      0,       // vertexBindingDescriptionCount
      nullptr, // pVertexBindingDescriptions
      0,       // vertexAttributeDescriptionCount
      nullptr, // pVertexAttributeDescriptions
  };

  auto inputAssemblyStateCreateInfo = pipeline::defaultInputAssemblyState();
  auto viewportStateCreateInfo = pipeline::defaultViewportState();
  auto rasterizationStateCreateInfo = pipeline::defaultRasterizationState();
  auto multisampleStateCreateInfo =
      pipeline::defaultMultisampleState(window.getSampleCount());
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
      m_pipelineLayout,              // pipelineLayout
      window.getRenderPass(),        // renderPass
      0,                             // subpass
      {},                            // basePipelineHandle
      -1                             // basePipelineIndex
  };

  VK_CHECK(vkCreateGraphicsPipelines(
      ctx().m_device,
      VK_NULL_HANDLE,
      1,
      &pipelineCreateInfo,
      nullptr,
      &m_pipeline));
}

SkyboxPipeline::SkyboxPipeline(Window &window, Shader &shader) {
  VkDescriptorSetLayout descriptorSetLayouts[] = {
      *ctx().m_descriptorManager.getSetLayout(renderer::DESC_CAMERA),
      *ctx().m_descriptorManager.getSetLayout(renderer::DESC_ENVIRONMENT),
  };

  m_pipelineLayout = pipeline::createPipelineLayout(
      ARRAYSIZE(descriptorSetLayouts), descriptorSetLayouts);

  auto shaderStageCreateInfos = shader.getPipelineShaderStageCreateInfos();

  VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, // sType
      nullptr,                                                   // pNext
      0,                                                         // flags
      0,       // vertexBindingDescriptionCount
      nullptr, // pVertexBindingDescriptions
      0,       // vertexAttributeDescriptionCount
      nullptr, // pVertexAttributeDescriptions
  };

  auto inputAssemblyStateCreateInfo = pipeline::defaultInputAssemblyState();
  auto viewportStateCreateInfo = pipeline::defaultViewportState();
  auto rasterizationStateCreateInfo = pipeline::defaultRasterizationState();
  rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE;
  auto multisampleStateCreateInfo =
      pipeline::defaultMultisampleState(window.getSampleCount());
  auto depthStencilStateCreateInfo = pipeline::defaultDepthStencilState();
  depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
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
      m_pipelineLayout,              // pipelineLayout
      window.getRenderPass(),        // renderPass
      0,                             // subpass
      {},                            // basePipelineHandle
      -1                             // basePipelineIndex
  };

  VK_CHECK(vkCreateGraphicsPipelines(
      ctx().m_device,
      VK_NULL_HANDLE,
      1,
      &pipelineCreateInfo,
      nullptr,
      &m_pipeline));
}

BakeCubemapPipeline::BakeCubemapPipeline(
    VkRenderPass &renderpass, Shader &shader) {
  VkDescriptorSetLayout descriptorSetLayouts[] = {
      *ctx().m_descriptorManager.getSetLayout(renderer::DESC_MATERIAL),
  };

  VkPushConstantRange pushConstantRange = {};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = 128;

  // Create pipeline layout
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      nullptr,
      0,
      ARRAYSIZE(descriptorSetLayouts), // setLayoutCount
      descriptorSetLayouts,            // pSetLayouts
      1,                               // pushConstantRangeCount
      &pushConstantRange,              // pPushConstantRanges
  };

  VK_CHECK(vkCreatePipelineLayout(
      ctx().m_device, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout));

  auto shaderStageCreateInfos = shader.getPipelineShaderStageCreateInfos();

  VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, // sType
      nullptr,                                                   // pNext
      0,                                                         // flags
      0,       // vertexBindingDescriptionCount
      nullptr, // pVertexBindingDescriptions
      0,       // vertexAttributeDescriptionCount
      nullptr, // pVertexAttributeDescriptions
  };

  auto inputAssemblyStateCreateInfo = pipeline::defaultInputAssemblyState();
  auto viewportStateCreateInfo = pipeline::defaultViewportState();
  auto rasterizationStateCreateInfo = pipeline::defaultRasterizationState();
  auto multisampleStateCreateInfo =
      pipeline::defaultMultisampleState(VK_SAMPLE_COUNT_1_BIT);
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
      m_pipelineLayout,              // pipelineLayout
      renderpass,                    // renderPass
      0,                             // subpass
      {},                            // basePipelineHandle
      -1                             // basePipelineIndex
  };

  VK_CHECK(vkCreateGraphicsPipelines(
      ctx().m_device,
      VK_NULL_HANDLE,
      1,
      &pipelineCreateInfo,
      nullptr,
      &m_pipeline));
}
} // namespace renderer

namespace renderer::pipeline {
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
      ctx().m_device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

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
