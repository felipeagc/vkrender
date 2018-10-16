#include "pipeline.hpp"
#include "context.hpp"
#include "logging.hpp"
#include "shader_compilation.hpp"
#include "window.hpp"
#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <spirv_reflect.hpp>

using namespace vkr;

VertexFormat::VertexFormat(
    SmallVec<vk::VertexInputBindingDescription> bindingDescriptions,
    SmallVec<vk::VertexInputAttributeDescription> attributeDescriptions)
    : bindingDescriptions(bindingDescriptions),
      attributeDescriptions(attributeDescriptions) {}

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
  this->bindingDescriptions.push_back({binding, stride, inputRate});
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

Shader::Shader(const std::string &vertexPath, const std::string &fragmentPath) {
  log::debug("Creating shader from GLSL code");
  this->vertexCode = compileShader(vertexPath, ShaderType::eVertex);
  this->fragmentCode = compileShader(fragmentPath, ShaderType::eFragment);
  this->vertexModule = this->createShaderModule(vertexCode);
  this->fragmentModule = this->createShaderModule(fragmentCode);
}

Shader::Shader(
    const std::vector<uint32_t> &vertexCode, const std::vector<uint32_t> &fragmentCode) {
  log::debug("Creating shader from SPV code");
  this->vertexCode = vertexCode;
  this->fragmentCode = fragmentCode;
  this->vertexModule = this->createShaderModule(vertexCode);
  this->fragmentModule = this->createShaderModule(fragmentCode);
}

SmallVec<vk::PipelineShaderStageCreateInfo>
Shader::getPipelineShaderStageCreateInfos() const {
  return SmallVec<vk::PipelineShaderStageCreateInfo>{
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

Shader::ShaderMetadata Shader::getAutoMetadata() const {
  Shader::ShaderMetadata metadata{};

  spirv_cross::Compiler vertexComp(
      this->vertexCode.data(), this->vertexCode.size());

  spirv_cross::Compiler fragmentComp(
      this->fragmentCode.data(), this->fragmentCode.size());

  // Descriptor stuff ===========================

  auto addBindings = [&](const spirv_cross::Compiler &comp,
                         vk::ShaderStageFlags shaderStage) {
    auto resources = comp.get_shader_resources();

    auto addBinding = [&](auto resources, auto type) {
      for (auto &res : resources) {
        metadata.descriptorSetLayoutBindings.push_back({
            comp.get_decoration(
                res.id, spv::Decoration::DecorationBinding), // binding
            type,                                            // descriptorType
            1,                                               // descriptorCount
            shaderStage,                                     // stageFlags
            nullptr, // pImmutableSamplers
        });
      }
    };

    addBinding(resources.separate_samplers, vk::DescriptorType::eSampler);
    addBinding(
        resources.sampled_images, vk::DescriptorType::eCombinedImageSampler);
    addBinding(resources.separate_images, vk::DescriptorType::eSampledImage);
    addBinding(resources.storage_images, vk::DescriptorType::eStorageImage);
    addBinding(resources.uniform_buffers, vk::DescriptorType::eUniformBuffer);
    addBinding(resources.storage_buffers, vk::DescriptorType::eStorageBuffer);
  };

  addBindings(vertexComp, vk::ShaderStageFlagBits::eVertex);
  addBindings(fragmentComp, vk::ShaderStageFlagBits::eFragment);

  // Vertex inputs ===========================

  auto resources = vertexComp.get_shader_resources();

  // <location, format, size, offset>
  SmallVec<std::tuple<uint32_t, vk::Format, uint32_t, uint32_t>> locations;

  for (auto &input : resources.stage_inputs) {
    auto location = vertexComp.get_decoration(
        input.id, spv::Decoration::DecorationLocation);
    auto type = vertexComp.get_type_from_variable(input.id);

    auto byteSize = (type.width * type.vecsize) / 8;

    std::array<vk::Format, 4> possibleFormats{vk::Format::eR32Sfloat,
                                              vk::Format::eR32G32Sfloat,
                                              vk::Format::eR32G32B32Sfloat,
                                              vk::Format::eR32G32B32A32Sfloat};

    locations.push_back(
        {location, possibleFormats[type.vecsize - 1], byteSize, 0});
  }

  std::sort(locations.begin(), locations.end(), [](auto &a, auto &b) {
    return std::get<0>(a) < std::get<0>(b);
  });

  for (size_t i = 0; i < locations.size(); i++) {
    if (i == 0) {
      // offset
      std::get<3>(locations[i]) = 0;
    } else {
      // Sum of the previous sizes
      uint32_t sum = 0;
      for (size_t j = 0; j < i; j++) {
        sum += std::get<2>(locations[j]);
      }

      // offset
      std::get<3>(locations[i]) = sum;
    }
  }

  uint32_t vertexSize = 0;
  for (auto &p : locations) {
    // sum of all sizes
    vertexSize += std::get<2>(p);
  }

  metadata.vertexFormat.bindingDescriptions.push_back(
      {0, vertexSize, vk::VertexInputRate::eVertex});

  for (size_t i = 0; i < locations.size(); i++) {
    metadata.vertexFormat.attributeDescriptions.push_back(
        {std::get<0>(locations[i]),
         0,
         std::get<1>(locations[i]),
         std::get<3>(locations[i])});
  }

  return metadata;
}

void Shader::destroy() {
  Context::getDevice().waitIdle();
  Context::getDevice().destroy(this->vertexModule);
  Context::getDevice().destroy(this->fragmentModule);
}

vk::ShaderModule
Shader::createShaderModule(const std::vector<uint32_t> &code) const {
  return Context::getDevice().createShaderModule({
      {},                             // flags
      code.size() * sizeof(uint32_t), // codeSize
      code.data(),                    // pCode
  });
}

DescriptorSetLayout::DescriptorSetLayout(
    const SmallVec<vk::DescriptorSetLayoutBinding> &bindings)
    : vk::DescriptorSetLayout(Context::getDevice().createDescriptorSetLayout(
          {{}, static_cast<uint32_t>(bindings.size()), bindings.data()})) {}

void DescriptorSetLayout::destroy() {
  Context::getDevice().waitIdle();
  Context::getDevice().destroy(*this);
}

DescriptorPool::DescriptorPool(
    uint32_t maxSets,
    const SmallVec<vk::DescriptorSetLayoutBinding> &bindings) {
  SmallVec<vk::DescriptorPoolSize> poolSizes;

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

  vk::DescriptorPool descriptorPool =
      Context::getDevice().createDescriptorPool({
          vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, // flags
          maxSets,                                              // maxSets
          static_cast<uint32_t>(poolSizes.size()),              // poolSizeCount
          poolSizes.data(),                                     // pPoolSizes
      });

  *this = static_cast<DescriptorPool &>(descriptorPool);
}

DescriptorPool::DescriptorPool(
    uint32_t maxSets, const SmallVec<vk::DescriptorPoolSize> &poolSizes)
    : vk::DescriptorPool(Context::getDevice().createDescriptorPool(
          {{},
           maxSets,
           static_cast<uint32_t>(poolSizes.size()),
           poolSizes.data()})) {}

SmallVec<DescriptorSet> DescriptorPool::allocateDescriptorSets(
    uint32_t setCount, DescriptorSetLayout &layout) {
  SmallVec<DescriptorSetLayout> layouts(setCount, layout);
  return this->allocateDescriptorSets(layouts);
}

SmallVec<DescriptorSet> DescriptorPool::allocateDescriptorSets(
    const SmallVec<DescriptorSetLayout> &layouts) {
  auto sets = Context::getDevice().allocateDescriptorSets(
      {*this, static_cast<uint32_t>(layouts.size()), layouts.data()});

  SmallVec<DescriptorSet> descriptorSets(sets.size());

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
    const VertexFormat &vertexFormat,
    const SmallVec<DescriptorSetLayout> &descriptorSetLayouts) {
  log::debug("Creating graphics pipeline");

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
