#pragma once

#include "descriptor_set.h"
#include "limits.h"
#include "render_target.h"
#include "shader.h"
#include "vulkan.h"
#include <gmath.h>

typedef struct re_window_t re_window_t;

typedef enum re_index_type_t {
  RE_INDEX_TYPE_UINT16 = VK_INDEX_TYPE_UINT16,
  RE_INDEX_TYPE_UINT32 = VK_INDEX_TYPE_UINT32,
} re_index_type_t;

typedef struct re_viewport_t {
  float x;
  float y;
  float width;
  float height;
  float min_depth;
  float max_depth;
} re_viewport_t;

typedef struct re_offset_2d_t {
  int32_t x;
  int32_t y;
} re_offset_2d_t;

typedef struct re_extent_2d_t {
  uint32_t width;
  uint32_t height;
} re_extent_2d_t;

typedef struct re_rect_2d_t {
  re_offset_2d_t offset;
  re_extent_2d_t extent;
} re_rect_2d_t;

typedef struct re_pipeline_parameters_t {
  VkPipelineVertexInputStateCreateInfo vertex_input_state;
  VkPipelineInputAssemblyStateCreateInfo input_assembly_state;
  VkPipelineViewportStateCreateInfo viewport_state;
  VkPipelineRasterizationStateCreateInfo rasterization_state;
  VkPipelineDepthStencilStateCreateInfo depth_stencil_state;
  VkPipelineColorBlendStateCreateInfo color_blend_state;
  VkPipelineDynamicStateCreateInfo dynamic_state;
} re_pipeline_parameters_t;

re_pipeline_parameters_t re_default_pipeline_parameters();

typedef struct re_vertex_t {
  vec3_t pos;
  vec3_t normal;
  vec2_t uv;
} re_vertex_t;

typedef struct re_pipeline_layout_t {
  VkPipelineLayout layout;

  re_descriptor_set_allocator_t
      *descriptor_set_allocators[RE_MAX_DESCRIPTOR_SETS];
  uint32_t descriptor_set_count;

  VkPushConstantRange push_constants[RE_MAX_PUSH_CONSTANT_RANGES];
  uint32_t push_constant_count;

  VkShaderStageFlagBits stage_flags[RE_MAX_SHADER_STAGES];
} re_pipeline_layout_t;

typedef struct re_pipeline_t {
  re_pipeline_layout_t layout;
  re_pipeline_parameters_t parameters;
  VkPipelineBindPoint bind_point;

  re_shader_t shaders[RE_MAX_SHADER_STAGES];
  uint32_t shader_count;

  uint32_t pipeline_count;
  struct {
    // TODO: replace this with a hash of the renderpass
    const re_render_target_t *render_target;
    VkPipeline pipeline;
  } pipelines[RE_MAX_RENDER_TARGETS];
} re_pipeline_t;

void re_pipeline_layout_init(
    re_pipeline_layout_t *layout, re_shader_t *shaders, uint32_t shader_count);

void re_pipeline_layout_destroy(re_pipeline_layout_t *layout);

void re_pipeline_init_graphics(
    re_pipeline_t *pipeline,
    re_shader_t *shaders,
    uint32_t shader_count,
    const re_pipeline_parameters_t parameters);

VkPipeline re_pipeline_get(
    re_pipeline_t *pipeline, const re_render_target_t *render_target);

void re_pipeline_destroy(re_pipeline_t *pipeline);
