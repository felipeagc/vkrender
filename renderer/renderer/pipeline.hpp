#pragma once

#include "glm.hpp"
#include <vulkan/vulkan.h>

namespace renderer {
class Window;
class Shader;

struct StandardVertex {
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 uv;
};

class GraphicsPipeline {
public:
  GraphicsPipeline(){};
  GraphicsPipeline(VkPipeline pipeline, VkPipelineLayout pipelineLayout);
  ~GraphicsPipeline();

  // GraphicsPipeline cannot be copied
  GraphicsPipeline(const GraphicsPipeline &) = delete;
  GraphicsPipeline &operator=(const GraphicsPipeline &) = delete;

  // GraphicsPipeline can be moved
  GraphicsPipeline(GraphicsPipeline &&rhs);
  GraphicsPipeline &operator=(GraphicsPipeline &&rhs);

  operator bool() { return m_pipeline != VK_NULL_HANDLE; }

  VkPipeline m_pipeline = VK_NULL_HANDLE;
  VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
};

class StandardPipeline : public GraphicsPipeline {
public:
  using GraphicsPipeline::GraphicsPipeline;
  StandardPipeline(Window &window, Shader &shader);
};

class BillboardPipeline : public GraphicsPipeline {
public:
  using GraphicsPipeline::GraphicsPipeline;
  BillboardPipeline(Window &window, Shader &shader);
};
} // namespace renderer

namespace renderer::pipeline {
VkPipelineLayout createPipelineLayout(
    uint32_t setLayoutCount, VkDescriptorSetLayout *descriptorSetLayouts);

VkPipelineVertexInputStateCreateInfo defaultVertexInputState();
VkPipelineInputAssemblyStateCreateInfo defaultInputAssemblyState();
VkPipelineViewportStateCreateInfo defaultViewportState();
VkPipelineRasterizationStateCreateInfo defaultRasterizationState();
VkPipelineMultisampleStateCreateInfo defaultMultisampleState(
    VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT);
VkPipelineDepthStencilStateCreateInfo defaultDepthStencilState();
VkPipelineColorBlendStateCreateInfo defaultColorBlendState();
VkPipelineDynamicStateCreateInfo defaultDynamicState();
} // namespace renderer::pipeline
