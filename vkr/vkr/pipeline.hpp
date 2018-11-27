#pragma once

#include <vulkan/vulkan.h>
#include <vkr/glm.hpp>

namespace vkr {
class Window;
class Shader;

struct StandardVertex {
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 uv;
};

class GraphicsPipeline {
public:
  GraphicsPipeline(VkPipeline pipeline, VkPipelineLayout pipelineLayout);
  ~GraphicsPipeline();

  // GraphicsPipeline cannot be copied
  GraphicsPipeline(const GraphicsPipeline &) = delete;
  GraphicsPipeline &operator=(const GraphicsPipeline &) = delete;

  // GraphicsPipeline can be moved
  GraphicsPipeline(GraphicsPipeline &&rhs);
  GraphicsPipeline &operator=(GraphicsPipeline &&rhs);

  VkPipeline m_pipeline = VK_NULL_HANDLE;
  VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
};

GraphicsPipeline createStandardPipeline(Window &window, Shader &shader);

GraphicsPipeline createBillboardPipeline(Window &window, Shader &shader);
} // namespace vkr

namespace vkr::pipeline {
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
} // namespace vkr::pipeline
