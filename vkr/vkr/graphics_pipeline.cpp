#include "graphics_pipeline.hpp"
#include "context.hpp"
#include "window.hpp"
#include "vertex_format.hpp"
#include "shader.hpp"
#include <fstl/logging.hpp>

using namespace vkr;

GraphicsPipeline::GraphicsPipeline(
    const Window &window,
    const Shader &shader,
    const VertexFormat &vertexFormat,
    const fstl::fixed_vector<DescriptorSetLayout> &descriptorSetLayouts) {
  fstl::log::debug("Creating graphics pipeline");

  vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo =
      vertexFormat.getPipelineVertexInputStateCreateInfo();

  auto shaderStageCreateInfos = shader.getPipelineShaderStageCreateInfos();

  vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{
      {},                                   // flags
      vk::PrimitiveTopology::eTriangleList, // topology
      VK_FALSE                              // primitiveRestartEnable
  };

  // pViewports and pScissors are null because we're defining them through a
  // dynamic state
  vk::PipelineViewportStateCreateInfo viewportStateCreateInfo{
      {},      // flags
      1,       // viewportCount
      nullptr, // pViewports
      1,       // scissorCount
      nullptr  // pScissors
  };

  vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{
      {},                               // flags
      VK_FALSE,                         // depthClampEnable
      VK_FALSE,                         // rasterizerDiscardEnable
      vk::PolygonMode::eFill,           // polygonMode
      vk::CullModeFlagBits::eFront,     // cullMode
      vk::FrontFace::eCounterClockwise, // frontFace
      VK_FALSE,                         // depthBiasEnable
      0.0f,                             // depthBiasConstantFactor,
      0.0f,                             // depthBiasClamp
      0.0f,                             // depthBiasSlopeFactor
      1.0f,                             // lineWidth
  };

  vk::Bool32 hasSampleShading =
      Context::getPhysicalDevice().getFeatures().sampleRateShading;

  vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo{
      {},                      // flags
      window.getMSAASamples(), // rasterizationSamples
      hasSampleShading,        // sampleShadingEnable
      0.25f,                   // minSampleShading
      nullptr,                 // pSampleMask
      VK_FALSE,                // alphaToCoverageEnable
      VK_FALSE                 // alphaToOneEnable
  };

  vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{
      {},                          // flags
      VK_TRUE,                     // depthTestEnable
      VK_TRUE,                     // depthWriteEnable
      vk::CompareOp::eLessOrEqual, // depthCompareOp
      VK_FALSE,                    // depthBoundsTestEnable
      VK_FALSE,                    // stencilTestEnable

      // Ignore stencil stuff
  };

  vk::PipelineColorBlendAttachmentState colorBlendAttachmentState = {
      VK_TRUE,                            // blendEnable
      vk::BlendFactor::eSrcAlpha,         // srcColorBlendFactor
      vk::BlendFactor::eOneMinusSrcAlpha, // dstColorBlendFactor
      vk::BlendOp::eAdd,                  // colorBlendOp
      vk::BlendFactor::eOneMinusSrcAlpha, // srcAlphaBlendFactor
      vk::BlendFactor::eZero,             // dstAlphaBlendFactor
      vk::BlendOp::eAdd,                  // alphaBlendOp
      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
          vk::ColorComponentFlagBits::eB |
          vk::ColorComponentFlagBits::eA // colorWriteMask
  };

  vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {
      {},                         // flags
      VK_FALSE,                   // logicOpEnable
      vk::LogicOp::eCopy,         // logicOp
      1,                          // attachmentCount
      &colorBlendAttachmentState, // pAttachments
      {0.0f, 0.0f, 0.0f, 0.0f},   // blendConstants
  };

  std::array<vk::DynamicState, 2> dynamicStates = {
      vk::DynamicState::eViewport,
      vk::DynamicState::eScissor,
  };

  vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo{
      {},                                          // flags
      static_cast<uint32_t>(dynamicStates.size()), // dynamicStateCount
      dynamicStates.data()                         // pDyanmicStates
  };

  this->pipelineLayout = Context::getDevice().createPipelineLayout({
      {},                                                 // flags
      static_cast<uint32_t>(descriptorSetLayouts.size()), // setLayoutCount
      descriptorSetLayouts.data(),                        // pSetLayouts
      0,       // pushConstantRangeCount
      nullptr, // pPushConstantRanges
  });

  vk::GraphicsPipelineCreateInfo pipelineCreateInfo{
      {},                                                   // flags
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
      window.renderPass,             // renderPass
      0,                             // subpass
      {},                            // basePipelineHandle
      -1                             // basePipelineIndex
  };

  Context::getDevice().createGraphicsPipelines(
      {}, 1, &pipelineCreateInfo, nullptr, &this->pipeline);
}

PipelineLayout GraphicsPipeline::getLayout() const {
  return this->pipelineLayout;
}

void GraphicsPipeline::destroy() {
  Context::getDevice().waitIdle();
  Context::getDevice().destroy(this->pipeline);
  Context::getDevice().destroy(this->pipelineLayout);
}
