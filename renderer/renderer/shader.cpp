#include "shader.hpp"
#include "context.hpp"
#include "shader_compilation.hpp"
#include "util.hpp"
#include <fstl/logging.hpp>
#include <spirv_reflect.hpp>

using namespace renderer;

Shader::Shader(const std::string &vertexPath, const std::string &fragmentPath) {
  fstl::log::debug("Creating shader from GLSL code");
  m_vertexCode = compileShader(vertexPath, ShaderType::eVertex);
  m_fragmentCode = compileShader(fragmentPath, ShaderType::eFragment);
  m_vertexModule = this->createShaderModule(m_vertexCode);
  m_fragmentModule = this->createShaderModule(m_fragmentCode);
}

Shader::Shader(
    const std::vector<uint32_t> &vertexCode,
    const std::vector<uint32_t> &fragmentCode) {
  fstl::log::debug("Creating shader from SPV code");
  m_vertexCode = vertexCode;
  m_fragmentCode = fragmentCode;
  m_vertexModule = this->createShaderModule(vertexCode);
  m_fragmentModule = this->createShaderModule(fragmentCode);
}

fstl::fixed_vector<VkPipelineShaderStageCreateInfo>
Shader::getPipelineShaderStageCreateInfos() const {
  return fstl::fixed_vector<VkPipelineShaderStageCreateInfo>{
      {
          VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // sType
          nullptr,                                             // pNext
          0,                                                   // flags
          VK_SHADER_STAGE_VERTEX_BIT,                          // stage
          m_vertexModule,                                 // module
          "main",                                              // pName
          nullptr, // pSpecializationInfo
      },
      {
          VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // sType
          nullptr,                                             // pNext
          0,                                                   // flags
          VK_SHADER_STAGE_FRAGMENT_BIT,                        // stage
          m_fragmentModule,                               // module
          "main",                                              // pName
          nullptr, // pSpecializationInfo
      },
  };
}

Shader::ShaderMetadata Shader::getAutoMetadata() const {
  Shader::ShaderMetadata metadata{};

  spirv_cross::Compiler vertexComp(
      m_vertexCode.data(), m_vertexCode.size());

  spirv_cross::Compiler fragmentComp(
      m_fragmentCode.data(), m_fragmentCode.size());

  // Descriptor stuff ===========================

  auto addBindings = [&](const spirv_cross::Compiler &comp,
                         VkShaderStageFlags shaderStage) {
    auto resources = comp.get_shader_resources();

    auto addBinding = [&](const std::vector<spirv_cross::Resource> &resources,
                          VkDescriptorType type) {
      for (auto &res : resources) {
        metadata.descriptorSetLayoutBindings.push_back(
            VkDescriptorSetLayoutBinding{
                comp.get_decoration(
                    res.id, spv::Decoration::DecorationBinding), // binding
                type,        // descriptorType
                1,           // descriptorCount
                shaderStage, // stageFlags
                nullptr,     // pImmutableSamplers
            });
      }
    };

    addBinding(resources.separate_samplers, VK_DESCRIPTOR_TYPE_SAMPLER);
    addBinding(
        resources.sampled_images, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    addBinding(resources.separate_images, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    addBinding(resources.storage_images, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    addBinding(resources.uniform_buffers, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    addBinding(resources.storage_buffers, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
  };

  addBindings(vertexComp, VK_SHADER_STAGE_VERTEX_BIT);
  addBindings(fragmentComp, VK_SHADER_STAGE_FRAGMENT_BIT);

  // Vertex inputs ===========================

  auto resources = vertexComp.get_shader_resources();

  // <location, format, size, offset>
  fstl::fixed_vector<std::tuple<uint32_t, VkFormat, uint32_t, uint32_t>>
      locations;

  for (auto &input : resources.stage_inputs) {
    auto location = vertexComp.get_decoration(
        input.id, spv::Decoration::DecorationLocation);
    auto type = vertexComp.get_type_from_variable(input.id);

    auto byteSize = (type.width * type.vecsize) / 8;

    VkFormat possibleFormats[4] = {VK_FORMAT_R32_SFLOAT,
                                   VK_FORMAT_R32G32_SFLOAT,
                                   VK_FORMAT_R32G32B32_SFLOAT,
                                   VK_FORMAT_R32G32B32A32_SFLOAT};

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

  metadata.vertexFormat.m_bindingDescriptions.push_back(
      {0, vertexSize, VK_VERTEX_INPUT_RATE_VERTEX});

  for (size_t i = 0; i < locations.size(); i++) {
    metadata.vertexFormat.m_attributeDescriptions.push_back(
        {std::get<0>(locations[i]),
         0,
         std::get<1>(locations[i]),
         std::get<3>(locations[i])});
  }

  return metadata;
}

void Shader::destroy() {
  VK_CHECK(vkDeviceWaitIdle(ctx().m_device));

  vkDestroyShaderModule(ctx().m_device, m_vertexModule, nullptr);
  vkDestroyShaderModule(ctx().m_device, m_fragmentModule, nullptr);
}

VkShaderModule
Shader::createShaderModule(const std::vector<uint32_t> &code) const {
  VkShaderModule shaderModule;

  VkShaderModuleCreateInfo createInfo = {
      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      nullptr,
      0,
      code.size() * sizeof(uint32_t),
      code.data()};

  VK_CHECK(vkCreateShaderModule(
      ctx().m_device, &createInfo, nullptr, &shaderModule));
  return shaderModule;
}
