#pragma once

#include "glm.hpp"
#include "render_target.hpp"
#include "shader.hpp"
#include <ftl/vector.hpp>
#include <vulkan/vulkan.h>

namespace renderer::pipeline {
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

namespace renderer {
class Window;
class Shader;

struct StandardVertex {
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 uv;
};

struct PipelineParameters {
  VkPipelineVertexInputStateCreateInfo vertexInputState =
      pipeline::defaultVertexInputState();
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
      pipeline::defaultInputAssemblyState();
  VkPipelineTessellationStateCreateInfo tessellationState;
  bool hasTesselationState = false;
  VkPipelineViewportStateCreateInfo viewportState =
      pipeline::defaultViewportState();
  VkPipelineRasterizationStateCreateInfo rasterizationState =
      pipeline::defaultRasterizationState();
  VkPipelineDepthStencilStateCreateInfo depthStencilState =
      pipeline::defaultDepthStencilState();
  VkPipelineColorBlendStateCreateInfo colorBlendState =
      pipeline::defaultColorBlendState();
  VkPipelineDynamicStateCreateInfo dynamicState =
      pipeline::defaultDynamicState();
  VkPipelineLayout layout;
};

struct GraphicsPipeline {
  GraphicsPipeline(){};
  GraphicsPipeline(
      const re_render_target_t render_target,
      const re_shader_t shader,
      const PipelineParameters &parameters);
  GraphicsPipeline(VkPipeline pipeline, VkPipelineLayout pipelineLayout);
  ~GraphicsPipeline();

  // GraphicsPipeline cannot be copied
  GraphicsPipeline(const GraphicsPipeline &) = delete;
  GraphicsPipeline &operator=(const GraphicsPipeline &) = delete;

  // GraphicsPipeline can be moved
  GraphicsPipeline(GraphicsPipeline &&rhs);
  GraphicsPipeline &operator=(GraphicsPipeline &&rhs);

  VkPipeline pipeline = VK_NULL_HANDLE;
  VkPipelineLayout layout = VK_NULL_HANDLE;
};
} // namespace renderer
