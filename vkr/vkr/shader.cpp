#include "shader.hpp"
#include "context.hpp"
#include "shader_compilation.hpp"
#include <spirv_reflect.hpp>
#include <fstl/logging.hpp>

using namespace vkr;

Shader::Shader(const std::string &vertexPath, const std::string &fragmentPath) {
  fstl::log::debug("Creating shader from GLSL code");
  this->vertexCode = compileShader(vertexPath, ShaderType::eVertex);
  this->fragmentCode = compileShader(fragmentPath, ShaderType::eFragment);
  this->vertexModule = this->createShaderModule(vertexCode);
  this->fragmentModule = this->createShaderModule(fragmentCode);
}

Shader::Shader(
    const std::vector<uint32_t> &vertexCode,
    const std::vector<uint32_t> &fragmentCode) {
  fstl::log::debug("Creating shader from SPV code");
  this->vertexCode = vertexCode;
  this->fragmentCode = fragmentCode;
  this->vertexModule = this->createShaderModule(vertexCode);
  this->fragmentModule = this->createShaderModule(fragmentCode);
}

fstl::fixed_vector<vk::PipelineShaderStageCreateInfo>
Shader::getPipelineShaderStageCreateInfos() const {
  return fstl::fixed_vector<vk::PipelineShaderStageCreateInfo>{
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
  fstl::fixed_vector<std::tuple<uint32_t, vk::Format, uint32_t, uint32_t>>
      locations;

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
