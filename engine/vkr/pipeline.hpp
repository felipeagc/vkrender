#pragma once

#include "buffer.hpp"
#include "smallvec.hpp"
#include "util.hpp"
#include <fstream>
#include <spirv_reflect.hpp>
#include <vulkan/vulkan.hpp>

namespace vkr {
class Context;
class GraphicsPipeline;
class Window;

class VertexFormat {
  friend class GraphicsPipeline;
  friend class Shader;

public:
  VertexFormat(){};
  VertexFormat(
      SmallVec<vk::VertexInputBindingDescription> bindingDescriptions,
      SmallVec<vk::VertexInputAttributeDescription> attributeDescriptions);
  ~VertexFormat(){};
  VertexFormat(const VertexFormat &other) = default;
  VertexFormat &operator=(VertexFormat &other) = default;

protected:
  SmallVec<vk::VertexInputBindingDescription> bindingDescriptions;
  SmallVec<vk::VertexInputAttributeDescription> attributeDescriptions;

  vk::PipelineVertexInputStateCreateInfo
  getPipelineVertexInputStateCreateInfo() const;
};

class VertexFormatBuilder {
public:
  VertexFormatBuilder(){};
  ~VertexFormatBuilder(){};
  VertexFormatBuilder(const VertexFormatBuilder &other) = default;
  VertexFormatBuilder &operator=(VertexFormatBuilder &other) = default;

  VertexFormatBuilder
  addBinding(uint32_t binding, uint32_t stride, vk::VertexInputRate inputRate);
  VertexFormatBuilder addAttribute(
      uint32_t location, uint32_t binding, vk::Format format, uint32_t offset);

  VertexFormat build();

private:
  SmallVec<vk::VertexInputBindingDescription> bindingDescriptions;
  SmallVec<vk::VertexInputAttributeDescription> attributeDescriptions;
};

class DescriptorSetLayout : public vk::DescriptorSetLayout {
public:
  DescriptorSetLayout(){};
  DescriptorSetLayout(const SmallVec<DescriptorSetLayoutBinding> &bindings);
  ~DescriptorSetLayout(){};
  DescriptorSetLayout(const DescriptorSetLayout &other) = default;
  DescriptorSetLayout &operator=(const DescriptorSetLayout &other) = default;

  void destroy();
};

class DescriptorPool : public vk::DescriptorPool {
public:
  DescriptorPool(){};
  // Create a descriptor pool with sizes derived from the bindings and maxSets
  DescriptorPool(
      uint32_t maxSets, const SmallVec<DescriptorSetLayoutBinding> &bindings);

  // Create a descriptor pool with manually specified poolSizes
  DescriptorPool(
      uint32_t maxSets, const SmallVec<DescriptorPoolSize> &poolSizes);
  ~DescriptorPool(){};

  DescriptorPool(const DescriptorPool &) = default;
  DescriptorPool &operator=(const DescriptorPool &) = default;

  // Allocate many descriptor sets with one layout
  SmallVec<DescriptorSet>
  allocateDescriptorSets(uint32_t setCount, DescriptorSetLayout &layout);

  // Allocate many descriptor sets with different layouts
  SmallVec<DescriptorSet>
  allocateDescriptorSets(const SmallVec<DescriptorSetLayout> &layouts);

  void destroy();
};

class Shader {
public:
  // Creates a shader from a path to a GLSL file
  Shader(const std::string &vertexPath, const std::string &fragmentPath);

  // Creates a shader from SPIR-V code
  Shader(
      const std::vector<uint32_t> &vertexCode,
      const std::vector<uint32_t> &fragmentCode);

  ~Shader(){};
  Shader(const Shader &other) = default;
  Shader &operator=(Shader &other) = delete;

  SmallVec<vk::PipelineShaderStageCreateInfo>
  getPipelineShaderStageCreateInfos() const;

  struct ShaderMetadata {
    SmallVec<vkr::DescriptorSetLayoutBinding> descriptorSetLayoutBindings;
    vkr::VertexFormat vertexFormat;
  };

  ShaderMetadata getAutoMetadata() const;

  void destroy();

  static std::vector<uint32_t> loadCode(const std::string &path) {
    std::ifstream file(path, std::ios::binary);

    if (file.fail()) {
      throw std::runtime_error("Failed to load shader code file");
    }

    std::streampos begin, end;
    begin = file.tellg();
    file.seekg(0, std::ios::end);
    end = file.tellg();

    std::vector<uint32_t> result(static_cast<size_t>(end - begin) / 4);

    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char *>(result.data()), end - begin);
    file.close();

    return result;
  }

private:
  std::vector<uint32_t> vertexCode;
  std::vector<uint32_t> fragmentCode;
  vk::ShaderModule vertexModule;
  vk::ShaderModule fragmentModule;

  vk::ShaderModule createShaderModule(const std::vector<uint32_t> &code) const;
};

class GraphicsPipeline {
  friend class CommandBuffer;

public:
  GraphicsPipeline(
      const Window &window,
      const Shader &shader,
      const VertexFormat &vertexFormat,
      const SmallVec<DescriptorSetLayout> &descriptorSetLayouts = {});
  ~GraphicsPipeline(){};
  GraphicsPipeline(const GraphicsPipeline &other) = default;
  GraphicsPipeline &operator=(GraphicsPipeline &other) = delete;

  PipelineLayout getLayout() const;

  void destroy();

private:
  vk::Pipeline pipeline;
  PipelineLayout pipelineLayout;
};
} // namespace vkr
