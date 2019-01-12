#pragma once

#include "glm.hpp"
#include "render_target.hpp"
#include "shader.hpp"
#include <vulkan/vulkan.h>

struct re_pipeline_parameters_t {
  VkPipelineVertexInputStateCreateInfo vertex_input_state;
  VkPipelineInputAssemblyStateCreateInfo input_assembly_state;
  VkPipelineViewportStateCreateInfo viewport_state;
  VkPipelineRasterizationStateCreateInfo rasterization_state;
  VkPipelineDepthStencilStateCreateInfo depth_stencil_state;
  VkPipelineColorBlendStateCreateInfo color_blend_state;
  VkPipelineDynamicStateCreateInfo dynamic_state;
  VkPipelineLayout layout;
};

// Does not have a default pipeline layout
re_pipeline_parameters_t re_default_pipeline_parameters();

struct re_vertex_t {
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 uv;
};

struct re_pipeline_t {
  VkPipeline pipeline;
  VkPipelineLayout layout;
};

void re_pipeline_init_graphics(
    re_pipeline_t *pipeline,
    const re_render_target_t render_target,
    const re_shader_t shader,
    const re_pipeline_parameters_t parameters);

void re_pipeline_destroy(re_pipeline_t *pipeline);
