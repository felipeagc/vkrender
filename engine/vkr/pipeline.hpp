#pragma once

#include "util.hpp"
#include <fstream>
#include <vulkan/vulkan.hpp>

namespace vkr {
class Context;
class GraphicsPipeline;

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
  VertexFormatBuilder();
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

class Shader : public Destroyable {
public:
  Shader(
      const Context &context,
      std::vector<char> vertexCode,
      std::vector<char> fragmentCode);
  ~Shader(){};
  Shader(const Shader &other) = default;
  Shader &operator=(Shader &other) = delete;

  std::vector<vk::PipelineShaderStageCreateInfo>
  getPipelineShaderStageCreateInfos() const;

  void destroy() override;

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
  const Context &context;

  vk::ShaderModule vertexModule;
  vk::ShaderModule fragmentModule;

  vk::ShaderModule
  createShaderModule(const Context &context, std::vector<char> code) const;
};

class GraphicsPipeline : public Destroyable {
  friend class CommandBuffer;

public:
  GraphicsPipeline(
      const Context &context, const Shader &shader, VertexFormat &vertexFormat);
  ~GraphicsPipeline(){};
  GraphicsPipeline(const GraphicsPipeline &other) = default;
  GraphicsPipeline &operator=(GraphicsPipeline &other) = delete;

  void destroy() override;

private:
  const Context &context;

  vk::Pipeline pipeline;
  vk::PipelineLayout pipelineLayout;
};
} // namespace vkr
