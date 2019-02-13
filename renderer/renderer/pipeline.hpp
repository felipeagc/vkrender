#pragma once

#include "render_target.hpp"
#include "shader.hpp"
#include <vkm/vkm.h>
#include <vulkan/vulkan.h>

typedef struct re_pipeline_parameters_t {
  VkPipelineVertexInputStateCreateInfo vertex_input_state;
  VkPipelineInputAssemblyStateCreateInfo input_assembly_state;
  VkPipelineViewportStateCreateInfo viewport_state;
  VkPipelineRasterizationStateCreateInfo rasterization_state;
  VkPipelineDepthStencilStateCreateInfo depth_stencil_state;
  VkPipelineColorBlendStateCreateInfo color_blend_state;
  VkPipelineDynamicStateCreateInfo dynamic_state;
  VkPipelineLayout layout;
} re_pipeline_parameters_t;

// Does not have a default pipeline layout
re_pipeline_parameters_t re_default_pipeline_parameters();

typedef struct re_vertex_t {
  vec3_t pos;
  vec3_t normal;
  vec2_t uv;
} re_vertex_t;

typedef struct re_pipeline_t {
  VkPipeline pipeline;
  VkPipelineLayout layout;
} re_pipeline_t;

void re_pipeline_init_graphics(
    re_pipeline_t *pipeline,
    const re_render_target_t render_target,
    const re_shader_t shader,
    const re_pipeline_parameters_t parameters);

void re_pipeline_bind_graphics(
    re_pipeline_t *pipeline, struct re_window_t *window);

void re_pipeline_destroy(re_pipeline_t *pipeline);
