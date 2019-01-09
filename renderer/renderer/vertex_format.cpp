#include "vertex_format.hpp"

using namespace renderer;

VertexFormat::VertexFormat(
    ftl::small_vector<VkVertexInputBindingDescription> bindingDescriptions,
    ftl::small_vector<VkVertexInputAttributeDescription> attributeDescriptions)
    : m_bindingDescriptions(bindingDescriptions),
      m_attributeDescriptions(attributeDescriptions) {}

VkPipelineVertexInputStateCreateInfo
VertexFormat::getPipelineVertexInputStateCreateInfo() const {
  return VkPipelineVertexInputStateCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, // sType
      nullptr,                                                   // pNext
      0,                                                         // flags
      static_cast<uint32_t>(
          m_bindingDescriptions.size()), // vertexBindingDescriptionCount
      m_bindingDescriptions.data(),      // pVertexBindingDescriptions
      static_cast<uint32_t>(
          m_attributeDescriptions.size()), // vertexAttributeDescriptionCount
      m_attributeDescriptions.data()       // pVertexAttributeDescriptions
  };
}

VertexFormatBuilder VertexFormatBuilder::addBinding(
    uint32_t binding, uint32_t stride, VkVertexInputRate inputRate) {
  m_bindingDescriptions.push_back({binding, stride, inputRate});
  return *this;
}

VertexFormatBuilder VertexFormatBuilder::addAttribute(
    uint32_t location, uint32_t binding, VkFormat format, uint32_t offset) {
  m_attributeDescriptions.push_back({location, binding, format, offset});
  return *this;
}

VertexFormat VertexFormatBuilder::build() {
  return {m_bindingDescriptions, m_attributeDescriptions};
}
