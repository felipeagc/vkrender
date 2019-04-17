#pragma once

#include "render_target.h"
#include "shader.h"
#include <gmath.h>
#include <vulkan/vulkan.h>

#define RE_MAX_DESCRIPTOR_SETS 8
#define RE_MAX_DESCRIPTOR_SET_BINDINGS 8
#define RE_MAX_PUSH_CONSTANT_RANGES 4

typedef struct re_window_t re_window_t;

typedef struct re_pipeline_parameters_t {
  VkPipelineVertexInputStateCreateInfo vertex_input_state;
  VkPipelineInputAssemblyStateCreateInfo input_assembly_state;
  VkPipelineViewportStateCreateInfo viewport_state;
  VkPipelineRasterizationStateCreateInfo rasterization_state;
  VkPipelineDepthStencilStateCreateInfo depth_stencil_state;
  VkPipelineColorBlendStateCreateInfo color_blend_state;
  VkPipelineDynamicStateCreateInfo dynamic_state;
} re_pipeline_parameters_t;

// Does not have a default pipeline layout
re_pipeline_parameters_t re_default_pipeline_parameters();

typedef struct re_vertex_t {
  vec3_t pos;
  vec3_t normal;
  vec2_t uv;
} re_vertex_t;

typedef union re_descriptor_update_info_t {
  VkDescriptorImageInfo image_info;
  VkDescriptorBufferInfo buffer_info;
} re_descriptor_update_info_t;

typedef struct re_pipeline_layout_t {
  VkPipelineLayout layout;

  VkDescriptorUpdateTemplate update_templates[RE_MAX_DESCRIPTOR_SETS];
  VkDescriptorSetLayout set_layouts[RE_MAX_DESCRIPTOR_SETS];
  uint32_t set_layout_count;

  VkPushConstantRange push_constants[RE_MAX_PUSH_CONSTANT_RANGES];
  uint32_t push_constant_count;
} re_pipeline_layout_t;

typedef struct re_pipeline_t {
  VkPipeline pipeline;
  re_pipeline_layout_t layout;
} re_pipeline_t;

void re_pipeline_layout_init(
    re_pipeline_layout_t *layout,
    const re_shader_t *vertex_shader,
    const re_shader_t *fragment_shader);

void re_pipeline_layout_destroy(re_pipeline_layout_t *layout);

void re_pipeline_init_graphics(
    re_pipeline_t *pipeline,
    const re_render_target_t *render_target,
    const re_shader_t *vertex_shader,
    const re_shader_t *fragment_shader,
    const re_pipeline_parameters_t parameters);

void re_pipeline_destroy(re_pipeline_t *pipeline);
