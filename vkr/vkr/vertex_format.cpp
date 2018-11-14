#include "vertex_format.hpp"

using namespace vkr;

VertexFormat::VertexFormat(
    fstl::fixed_vector<VkVertexInputBindingDescription> bindingDescriptions,
    fstl::fixed_vector<VkVertexInputAttributeDescription> attributeDescriptions)
    : bindingDescriptions_(bindingDescriptions),
      attributeDescriptions_(attributeDescriptions) {}

VkPipelineVertexInputStateCreateInfo
VertexFormat::getPipelineVertexInputStateCreateInfo() const {
  return VkPipelineVertexInputStateCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, // sType
      nullptr,                                                   // pNext
      0,                                                         // flags
      static_cast<uint32_t>(
          this->bindingDescriptions_.size()), // vertexBindingDescriptionCount
      this->bindingDescriptions_.data(),      // pVertexBindingDescriptions
      static_cast<uint32_t>(this->attributeDescriptions_
                                .size()), // vertexAttributeDescriptionCount
      this->attributeDescriptions_.data() // pVertexAttributeDescriptions
  };
}

VertexFormatBuilder VertexFormatBuilder::addBinding(
    uint32_t binding, uint32_t stride, VkVertexInputRate inputRate) {
  this->bindingDescriptions_.push_back({binding, stride, inputRate});
  return *this;
}

VertexFormatBuilder VertexFormatBuilder::addAttribute(
    uint32_t location, uint32_t binding, VkFormat format, uint32_t offset) {
  this->attributeDescriptions_.push_back({location, binding, format, offset});
  return *this;
}

VertexFormat VertexFormatBuilder::build() {
  return {this->bindingDescriptions_, this->attributeDescriptions_};
}
