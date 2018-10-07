#include "pipeline.hpp"
#include "context.hpp"
#include "window.hpp"
#include "logging.hpp"
#include <cassert>
#include <functional>
#include <iostream>
#include <spirv_reflect.hpp>

using namespace vkr;

#define ADD_BINDING(type_spv, type_vk)                                         \
  for (auto &res : resources.type_spv) {                                       \
    bindings.push_back({                                                       \
        comp.get_decoration(res.id, spv::Decoration::DecorationBinding),       \
        vk::DescriptorType::type_vk,                                           \
        1,                                                                     \
        shaderStage,                                                           \
    });                                                                        \
  }

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

Shader::Shader(std::vector<char> vertexCode, std::vector<char> fragmentCode) {
  log::debug("Creating shader from SPV code");
  this->vertexCode = vertexCode;
  this->fragmentCode = fragmentCode;
  this->vertexModule = this->createShaderModule(vertexCode);
  this->fragmentModule = this->createShaderModule(fragmentCode);
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

std::vector<vkr::DescriptorSetLayoutBinding>
Shader::getDescriptorSetLayoutBindings() const {
  std::vector<vkr::DescriptorSetLayoutBinding> bindings;

  auto addBindings = [&](std::vector<char> code,
                         vk::ShaderStageFlags shaderStage) {
    spirv_cross::Compiler comp(
        reinterpret_cast<uint32_t *>(code.data()),
        code.size() / sizeof(uint32_t));

    auto resources = comp.get_shader_resources();

    ADD_BINDING(separate_samplers, eSampler);
    ADD_BINDING(sampled_images, eCombinedImageSampler);
    ADD_BINDING(separate_images, eSampledImage);
    ADD_BINDING(storage_images, eStorageImage);
    ADD_BINDING(uniform_buffers, eUniformBuffer);
    ADD_BINDING(storage_buffers, eStorageBuffer);
  };

  addBindings(this->vertexCode, vk::ShaderStageFlagBits::eVertex);
  addBindings(this->fragmentCode, vk::ShaderStageFlagBits::eFragment);

  return bindings;
}

void Shader::destroy() {
  Context::getDevice().waitIdle();
  Context::getDevice().destroy(this->vertexModule);
  Context::getDevice().destroy(this->fragmentModule);
}

vk::ShaderModule Shader::createShaderModule(std::vector<char> code) const {
  return Context::getDevice().createShaderModule({
      {},                                              // flags
      code.size(),                                     // codeSize
      reinterpret_cast<const uint32_t *>(code.data()), // pCode
  });
}

DescriptorSetLayout::DescriptorSetLayout(
    std::vector<vk::DescriptorSetLayoutBinding> bindings)
    : vk::DescriptorSetLayout(Context::getDevice().createDescriptorSetLayout(
          {{}, static_cast<uint32_t>(bindings.size()), bindings.data()})) {
}

void DescriptorSetLayout::destroy() {
  Context::getDevice().waitIdle();
  Context::getDevice().destroy(*this);
}

DescriptorPool::DescriptorPool(
    uint32_t maxSets, std::vector<vk::DescriptorSetLayoutBinding> bindings) {
  std::vector<vk::DescriptorPoolSize> poolSizes;

  for (auto &binding : bindings) {
    vk::DescriptorPoolSize *foundp = nullptr;
    for (auto &poolSize : poolSizes) {
      if (poolSize.type == binding.descriptorType) {
        foundp = &poolSize;
        break;
      }
    }

    if (foundp == nullptr) {
      poolSizes.push_back({
          binding.descriptorType,
          binding.descriptorCount,
      });
    } else {
      foundp->descriptorCount += binding.descriptorCount;
    }
  }

  for (auto &poolSize : poolSizes) {
    poolSize.descriptorCount *= maxSets;
  }

  vk::DescriptorPool descriptorPool = Context::getDevice().createDescriptorPool(
      {{}, maxSets, static_cast<uint32_t>(poolSizes.size()), poolSizes.data()});

  *this = static_cast<DescriptorPool &>(descriptorPool);
}

DescriptorPool::DescriptorPool(
    uint32_t maxSets, std::vector<vk::DescriptorPoolSize> poolSizes)
    : vk::DescriptorPool(Context::getDevice().createDescriptorPool(
          {{},
           maxSets,
           static_cast<uint32_t>(poolSizes.size()),
           poolSizes.data()})) {
}

std::vector<DescriptorSet> DescriptorPool::allocateDescriptorSets(
    uint32_t setCount, DescriptorSetLayout layout) {
  std::vector<DescriptorSetLayout> layouts(setCount, layout);
  return this->allocateDescriptorSets(layouts);
}

std::vector<DescriptorSet> DescriptorPool::allocateDescriptorSets(
    std::vector<DescriptorSetLayout> layouts) {
  auto sets = Context::getDevice().allocateDescriptorSets(
      {*this, static_cast<uint32_t>(layouts.size()), layouts.data()});

  std::vector<DescriptorSet> descriptorSets(sets.size());

  for (size_t i = 0; i < sets.size(); i++) {
    descriptorSets[i] = {sets[i]};
  }

  return descriptorSets;
}

void DescriptorPool::destroy() {
  Context::getDevice().waitIdle();
  Context::getDevice().destroy(*this);
}

GraphicsPipeline::GraphicsPipeline(
    const Window &window,
    const Shader &shader,
    VertexFormat &vertexFormat,
    std::vector<DescriptorSetLayout> descriptorSetLayouts) {
  log::debug("Creating graphics pipeline");

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
