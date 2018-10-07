#pragma once

#include "buffer.hpp"
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

public:
  VertexFormat(
      std::vector<vk::VertexInputBindingDescription> bindingDescriptions,
      std::vector<vk::VertexInputAttributeDescription> attributeDescriptions);
  ~VertexFormat(){};
  VertexFormat(const VertexFormat &other) = default;
  VertexFormat &operator=(VertexFormat &other) = default;

protected:
  std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
  std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;

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
  std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
  std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
};

class DescriptorSetLayout : public vk::DescriptorSetLayout {
public:
  DescriptorSetLayout(std::vector<DescriptorSetLayoutBinding> bindings);
  ~DescriptorSetLayout(){};
  DescriptorSetLayout(const DescriptorSetLayout &other) = default;
  DescriptorSetLayout &operator=(const DescriptorSetLayout &other) = default;

  void destroy();
};

class DescriptorSet : public vk::DescriptorSet {
public:
  using vk::DescriptorSet::DescriptorSet;
};

class DescriptorPool : public vk::DescriptorPool {
public:
  // Create a descriptor pool with sizes derived from the bindings and maxSets
  DescriptorPool(
      uint32_t maxSets, std::vector<DescriptorSetLayoutBinding> bindings);

  // Create a descriptor pool with manually specified poolSizes
  DescriptorPool(uint32_t maxSets, std::vector<DescriptorPoolSize> poolSizes);
  ~DescriptorPool(){};

  DescriptorPool(const DescriptorPool &) = default;

  // Allocate many descriptor sets with one layout
  std::vector<DescriptorSet>
  allocateDescriptorSets(uint32_t setCount, DescriptorSetLayout layout);

  // Allocate many descriptor sets with different layouts
  std::vector<DescriptorSet>
  allocateDescriptorSets(std::vector<DescriptorSetLayout> layouts);

  void destroy();

private:
  DescriptorPool &operator=(DescriptorPool &) = default;
};

class Shader {
public:
  Shader(std::vector<char> vertexCode, std::vector<char> fragmentCode);
  ~Shader(){};
  Shader(const Shader &other) = default;
  Shader &operator=(Shader &other) = delete;

  std::vector<vk::PipelineShaderStageCreateInfo>
  getPipelineShaderStageCreateInfos() const;

  std::vector<vkr::DescriptorSetLayoutBinding> getDescriptorSetLayoutBindings() const;

  void destroy();

  static std::vector<char> loadCode(const std::string &path) {
    std::ifstream file(path, std::ios::binary);

    if (file.fail()) {
      throw std::runtime_error("Failed to load shader code file");
    }

    std::streampos begin, end;
    begin = file.tellg();
    file.seekg(0, std::ios::end);
    end = file.tellg();

    std::vector<char> result(static_cast<size_t>(end - begin));

    file.seekg(0, std::ios::beg);
    file.read(result.data(), end - begin);
    file.close();

    return result;
  }

private:
  std::vector<char> vertexCode;
  std::vector<char> fragmentCode;
  vk::ShaderModule vertexModule;
  vk::ShaderModule fragmentModule;

  vk::ShaderModule createShaderModule(std::vector<char> code) const;

};

class GraphicsPipeline {
  friend class CommandBuffer;

public:
  GraphicsPipeline(
      const Window &window,
      const Shader &shader,
      VertexFormat &vertexFormat,
      std::vector<DescriptorSetLayout> descriptorSetLayouts =
          std::vector<DescriptorSetLayout>());
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
