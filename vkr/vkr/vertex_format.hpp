#pragma once

#include <fstl/fixed_vector.hpp>

namespace vkr {
class VertexFormat {
  friend class GraphicsPipeline;
  friend class Shader;

public:
  VertexFormat(){};
  VertexFormat(
      fstl::fixed_vector<vk::VertexInputBindingDescription> bindingDescriptions,
      fstl::fixed_vector<vk::VertexInputAttributeDescription>
          attributeDescriptions);
  ~VertexFormat(){};
  VertexFormat(const VertexFormat &other) = default;
  VertexFormat &operator=(VertexFormat &other) = default;

protected:
  fstl::fixed_vector<vk::VertexInputBindingDescription> bindingDescriptions;
  fstl::fixed_vector<vk::VertexInputAttributeDescription> attributeDescriptions;

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
  fstl::fixed_vector<vk::VertexInputBindingDescription> bindingDescriptions;
  fstl::fixed_vector<vk::VertexInputAttributeDescription> attributeDescriptions;
};

} // namespace vkr
