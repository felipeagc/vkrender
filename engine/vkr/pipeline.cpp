#include "pipeline.hpp"
#include "context.hpp"

using namespace vkr;

VertexFormat::VertexFormat(
    std::vector<vk::VertexInputBindingDescription> bindingDescriptions,
    std::vector<vk::VertexInputAttributeDescription> attributeDescriptions)
    : bindingDescriptions(bindingDescriptions),
      attributeDescriptions(attributeDescriptions) {
}

vk::PipelineVertexInputStateCreateInfo
VertexFormat::getPipelineVertexInputStateCreateInfo() const {
  return vk::PipelineVertexInputStateCreateInfo{
      {}, // flags
      static_cast<uint32_t>(
          this->bindingDescriptions.size()), // vertexBindingDescriptionCount
      this->bindingDescriptions.data(),      // pVertexBindingDescriptions
      static_cast<uint32_t>(this->attributeDescriptions
                                .size()), // vertexAttributeDescriptionCount
      this->attributeDescriptions.data()  // pVertexAttributeDescriptions
  };
}

VertexFormatBuilder::VertexFormatBuilder() {
}

VertexFormatBuilder VertexFormatBuilder::addBinding(
    uint32_t binding, uint32_t stride, vk::VertexInputRate inputRate) {
  this->bindingDescriptions.push_back({0, stride, inputRate});
  return *this;
}

VertexFormatBuilder VertexFormatBuilder::addAttribute(
    uint32_t location, uint32_t binding, vk::Format format, uint32_t offset) {
  this->attributeDescriptions.push_back({location, binding, format, offset});
  return *this;
}

VertexFormat VertexFormatBuilder::build() {
  return {this->bindingDescriptions, this->attributeDescriptions};
}

Shader::Shader(
    const Context &context,
    std::vector<char> vertexCode,
    std::vector<char> fragmentCode)
    : context(context) {
  this->vertexModule = this->createShaderModule(context, vertexCode);
  this->fragmentModule = this->createShaderModule(context, fragmentCode);
}

std::vector<vk::PipelineShaderStageCreateInfo>
Shader::getPipelineShaderStageCreateInfos() const {
  return std::vector<vk::PipelineShaderStageCreateInfo>{
      {
          {},                               // flags
          vk::ShaderStageFlagBits::eVertex, // stage
          this->vertexModule,               // module
          "main",                           // pName
          nullptr,                          // pSpecializationInfo
      },
      {
          {},                                 // flags
          vk::ShaderStageFlagBits::eFragment, // stage
          this->fragmentModule,               // module
          "main",                             // pName
          nullptr,                            // pSpecializationInfo
      },
  };
}

void Shader::destroy() {
  this->context.device.destroy(this->vertexModule);
  this->context.device.destroy(this->fragmentModule);
}

vk::ShaderModule Shader::createShaderModule(
    const Context &context, std::vector<char> code) const {
  return context.device.createShaderModule({
      {},                                              // flags
      code.size(),                                     // codeSize
      reinterpret_cast<const uint32_t *>(code.data()), // pCode
  });
}

GraphicsPipeline::GraphicsPipeline(
    const Context &context, const Shader &shader, VertexFormat &vertexFormat)
    : context(context) {
  vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo =
      vertexFormat.getPipelineVertexInputStateCreateInfo();

  std::vector<vk::PipelineShaderStageCreateInfo> shaderStageCreateInfos =
      shader.getPipelineShaderStageCreateInfos();

  vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{
      {},                                    // flags
      vk::PrimitiveTopology::eTriangleStrip, // topology
      VK_FALSE                               // primitiveRestartEnable
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
      vk::CullModeFlagBits::eNone,      // cullMode
      vk::FrontFace::eCounterClockwise, // frontFace
      VK_FALSE,                         // depthBiasEnable
      0.0f,                             // depthBiasConstantFactor,
      0.0f,                             // depthBiasClamp
      0.0f,                             // depthBiasSlopeFactor
      1.0f,                             // lineWidth
  };

  vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo{
      {},                          // flags
      vk::SampleCountFlagBits::e1, // rasterizationSamples
      VK_FALSE,                    // sampleShadingEnable
      1.0f,                        // minSampleShading
      nullptr,                     // pSampleMask
      VK_FALSE,                    // alphaToCoverageEnable
      VK_FALSE                     // alphaToOneEnable
  };

  vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{
      {},                   // flags
      VK_TRUE,              // depthTestEnable
      VK_TRUE,              // depthWriteEnable
      vk::CompareOp::eLess, // depthCompareOp
      VK_FALSE,             // depthBoundsTestEnable
      VK_FALSE,             // stencilTestEnable

      // Ignore stencil stuff
  };

  vk::PipelineColorBlendAttachmentState colorBlendAttachmentState = {
      VK_FALSE,               // blendEnable
      vk::BlendFactor::eOne,  // srcColorBlendFactor
      vk::BlendFactor::eZero, // dstColorBlendFactor
      vk::BlendOp::eAdd,      // colorBlendOp
      vk::BlendFactor::eOne,  // srcAlphaBlendFactor
      vk::BlendFactor::eZero, // dstAlphaBlendFactor
      vk::BlendOp::eAdd,      // alphaBlendOp
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

  this->pipelineLayout = context.device.createPipelineLayout({
      {},      // flags
      0,       // setLayoutCount
      nullptr, // pSetLayouts
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
      context.renderPass,            // renderPass
      0,                             // subpass
      {},                            // basePipelineHandle
      -1                             // basePipelineIndex
  };

  this->context.device.createGraphicsPipelines(
      {}, 1, &pipelineCreateInfo, nullptr, &this->pipeline);
}

void GraphicsPipeline::destroy() {
  this->context.device.waitIdle();
  this->context.device.destroy(this->pipeline);
  this->context.device.destroy(this->pipelineLayout);
}
