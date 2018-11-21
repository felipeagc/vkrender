#pragma once

#include <vulkan/vulkan.h>

namespace vkr {
class Window;
class Shader;

struct StandardVertex {
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 uv;
};

struct GraphicsPipeline {
  VkPipeline pipeline;
  VkPipelineLayout pipelineLayout;

  void destroy();
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
