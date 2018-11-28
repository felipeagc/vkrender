#pragma once

#include <fstl/fixed_vector.hpp>
#include <vulkan/vulkan.h>

namespace renderer {
class VertexFormat {
  friend class Shader;

public:
  VertexFormat(){};
  VertexFormat(
      fstl::fixed_vector<VkVertexInputBindingDescription> bindingDescriptions,
      fstl::fixed_vector<VkVertexInputAttributeDescription>
          attributeDescriptions);
  ~VertexFormat(){};
  VertexFormat(const VertexFormat &other) = default;
  VertexFormat &operator=(VertexFormat &other) = default;

  VkPipelineVertexInputStateCreateInfo
  getPipelineVertexInputStateCreateInfo() const;

protected:
  fstl::fixed_vector<VkVertexInputBindingDescription> m_bindingDescriptions;
  fstl::fixed_vector<VkVertexInputAttributeDescription> m_attributeDescriptions;
};

class VertexFormatBuilder {
public:
  VertexFormatBuilder(){};
  ~VertexFormatBuilder(){};
  VertexFormatBuilder(const VertexFormatBuilder &other) = default;
  VertexFormatBuilder &operator=(VertexFormatBuilder &other) = default;

  VertexFormatBuilder
  addBinding(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate);
  VertexFormatBuilder addAttribute(
      uint32_t location, uint32_t binding, VkFormat format, uint32_t offset);

  VertexFormat build();

private:
  fstl::fixed_vector<VkVertexInputBindingDescription> m_bindingDescriptions;
  fstl::fixed_vector<VkVertexInputAttributeDescription> m_attributeDescriptions;
};

} // namespace renderer
