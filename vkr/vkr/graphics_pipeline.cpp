#include "graphics_pipeline.hpp"
#include "context.hpp"
#include "shader.hpp"
#include "util.hpp"
#include "vertex_format.hpp"
#include "window.hpp"
#include <fstl/logging.hpp>

using namespace vkr;

GraphicsPipeline::GraphicsPipeline(
    const Window &window,
    const Shader &shader,
    const VertexFormat &vertexFormat,
    const fstl::fixed_vector<VkDescriptorSetLayout> &descriptorSetLayouts) {
  fstl::log::debug("Creating graphics pipeline");

  VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo =
      vertexFormat.getPipelineVertexInputStateCreateInfo();

  auto shaderStageCreateInfos = shader.getPipelineShaderStageCreateInfos();

  VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      nullptr,
      0,                                   // flags
      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, // topology
      VK_FALSE                             // primitiveRestartEnable
  };

  // pViewports and pScissors are null because we're defining them through a
  // dynamic state
  VkPipelineViewportStateCreateInfo viewportStateCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      nullptr,
      0,       // flags
      1,       // viewportCount
      nullptr, // pViewports
      1,       // scissorCount
      nullptr  // pScissors
  };

  VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{
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

  VkPhysicalDeviceFeatures deviceFeatures;
  vkGetPhysicalDeviceFeatures(ctx::physicalDevice, &deviceFeatures);
  VkBool32 hasSampleShading = deviceFeatures.sampleRateShading;

  VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      nullptr,
      0,                       // flags
      window.getMSAASamples(), // rasterizationSamples
      hasSampleShading,        // sampleShadingEnable
      0.25f,                   // minSampleShading
      nullptr,                 // pSampleMask
      VK_FALSE,                // alphaToCoverageEnable
      VK_FALSE                 // alphaToOneEnable
  };

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

  VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {
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

  VkDynamicState dynamicStates[] = {
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

  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      nullptr,
      0,
      static_cast<uint32_t>(descriptorSetLayouts.size()), // setLayoutCount
      descriptorSetLayouts.data(),                        // pSetLayouts
      0,       // pushConstantRangeCount
      nullptr, // pPushConstantRanges
  };

  VK_CHECK(vkCreatePipelineLayout(
      ctx::device,
      &pipelineLayoutCreateInfo,
      nullptr,
      &this->pipelineLayout_));

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
      pipelineLayout_,               // pipelineLayout
      window.renderPass_,            // renderPass
      0,                             // subpass
      {},                            // basePipelineHandle
      -1                             // basePipelineIndex
  };

  VK_CHECK(vkCreateGraphicsPipelines(
      ctx::device,
      VK_NULL_HANDLE,
      1,
      &pipelineCreateInfo,
      nullptr,
      &this->pipeline_));
}

VkPipelineLayout GraphicsPipeline::getLayout() const {
  return this->pipelineLayout_;
}

VkPipeline GraphicsPipeline::getPipeline() const { return this->pipeline_; }

void GraphicsPipeline::destroy() {
  VK_CHECK(vkDeviceWaitIdle(ctx::device));
  vkDestroyPipeline(ctx::device, this->pipeline_, nullptr);
  vkDestroyPipelineLayout(ctx::device, this->pipelineLayout_, nullptr);
}
