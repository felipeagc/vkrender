#include "pipelines.h"
#include "filesystem.h"
#include <renderer/context.h>
#include <renderer/util.h>
#include <renderer/window.h>

eg_default_pipeline_layouts_t g_default_pipeline_layouts;

static void init_pipeline_layout_spv(
    re_pipeline_layout_t *layout,
    const char *vertex_path,
    const char *fragment_path) {
  eg_file_t *vertex_file = eg_file_open_read(vertex_path);
  assert(vertex_file);
  size_t vertex_size = eg_file_size(vertex_file);
  unsigned char *vertex_code = calloc(1, vertex_size);
  eg_file_read_bytes(vertex_file, vertex_code, vertex_size);
  eg_file_close(vertex_file);

  eg_file_t *fragment_file = eg_file_open_read(fragment_path);
  assert(fragment_file);
  size_t fragment_size = eg_file_size(fragment_file);
  unsigned char *fragment_code = calloc(1, fragment_size);
  eg_file_read_bytes(fragment_file, fragment_code, fragment_size);
  eg_file_close(fragment_file);

  re_shader_t vertex_shader;
  re_shader_init_spv(&vertex_shader, (uint32_t *)vertex_code, vertex_size);

  re_shader_t fragment_shader;
  re_shader_init_spv(
      &fragment_shader, (uint32_t *)fragment_code, fragment_size);

  re_pipeline_layout_init(layout, &vertex_shader, &fragment_shader);

  free(vertex_code);
  free(fragment_code);

  re_shader_destroy(&fragment_shader);
  re_shader_destroy(&vertex_shader);
}

void eg_default_pipeline_layouts_init() {
  init_pipeline_layout_spv(
      &g_default_pipeline_layouts.pbr,
      "/shaders/pbr.vert.spv",
      "/shaders/pbr.frag.spv");

  init_pipeline_layout_spv(
      &g_default_pipeline_layouts.skybox,
      "/shaders/skybox.vert.spv",
      "/shaders/skybox.frag.spv");
}

void eg_default_pipeline_layouts_destroy() {
  re_pipeline_layout_destroy(&g_default_pipeline_layouts.pbr);
  re_pipeline_layout_destroy(&g_default_pipeline_layouts.skybox);
}

void eg_init_pipeline_spv(
    re_pipeline_t *pipeline,
    const re_render_target_t *render_target,
    const char *vertex_path,
    const char *fragment_path,
    const re_pipeline_parameters_t params) {
  eg_file_t *vertex_file = eg_file_open_read(vertex_path);
  assert(vertex_file);
  size_t vertex_size = eg_file_size(vertex_file);
  unsigned char *vertex_code = calloc(1, vertex_size);
  eg_file_read_bytes(vertex_file, vertex_code, vertex_size);
  eg_file_close(vertex_file);

  eg_file_t *fragment_file = eg_file_open_read(fragment_path);
  assert(fragment_file);
  size_t fragment_size = eg_file_size(fragment_file);
  unsigned char *fragment_code = calloc(1, fragment_size);
  eg_file_read_bytes(fragment_file, fragment_code, fragment_size);
  eg_file_close(fragment_file);

  re_shader_t vertex_shader;
  re_shader_init_spv(&vertex_shader, (uint32_t *)vertex_code, vertex_size);

  re_shader_t fragment_shader;
  re_shader_init_spv(
      &fragment_shader, (uint32_t *)fragment_code, fragment_size);

  re_pipeline_init_graphics(
      pipeline, render_target, &vertex_shader, &fragment_shader, params);

  free(vertex_code);
  free(fragment_code);

  re_shader_destroy(&fragment_shader);
  re_shader_destroy(&vertex_shader);
}

re_pipeline_parameters_t eg_pbr_pipeline_parameters() {
  re_pipeline_parameters_t params = re_default_pipeline_parameters();

  params.rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  params.rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;

  return params;
}

re_pipeline_parameters_t eg_picking_pipeline_parameters() {
  re_pipeline_parameters_t params = re_default_pipeline_parameters();

  params.rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  params.rasterization_state.cullMode = VK_CULL_MODE_NONE;

  return params;
}

re_pipeline_parameters_t eg_billboard_pipeline_parameters() {
  re_pipeline_parameters_t params = re_default_pipeline_parameters();

  params.vertex_input_state = (VkPipelineVertexInputStateCreateInfo){
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, // sType
      NULL,                                                      // pNext
      0,                                                         // flags
      0,    // vertexBindingDescriptionCount
      NULL, // pVertexBindingDescriptions
      0,    // vertexAttributeDescriptionCount
      NULL, // pVertexAttributeDescriptions
  };

  params.rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;
  params.rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

  return params;
}

re_pipeline_parameters_t eg_wireframe_pipeline_parameters() {
  re_pipeline_parameters_t params = re_default_pipeline_parameters();

  params.rasterization_state.cullMode = VK_CULL_MODE_NONE;
  params.rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  params.rasterization_state.polygonMode = VK_POLYGON_MODE_LINE;
  params.rasterization_state.lineWidth = 2.0f;

  return params;
}

re_pipeline_parameters_t eg_skybox_pipeline_parameters() {
  re_pipeline_parameters_t params = re_default_pipeline_parameters();

  params.vertex_input_state = (VkPipelineVertexInputStateCreateInfo){
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, // sType
      NULL,                                                      // pNext
      0,                                                         // flags
      0,    // vertexBindingDescriptionCount
      NULL, // pVertexBindingDescriptions
      0,    // vertexAttributeDescriptionCount
      NULL, // pVertexAttributeDescriptions
  };

  params.rasterization_state.cullMode = VK_CULL_MODE_NONE;

  params.depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

  return params;
}

re_pipeline_parameters_t eg_fullscreen_pipeline_parameters() {
  re_pipeline_parameters_t params = re_default_pipeline_parameters();

  params.vertex_input_state = (VkPipelineVertexInputStateCreateInfo){
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, // sType
      NULL,                                                      // pNext
      0,                                                         // flags
      0,    // vertexBindingDescriptionCount
      NULL, // pVertexBindingDescriptions
      0,    // vertexAttributeDescriptionCount
      NULL, // pVertexAttributeDescriptions
  };

  params.rasterization_state.cullMode = VK_CULL_MODE_FRONT_BIT;
  params.rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

  params.depth_stencil_state.depthTestEnable = VK_FALSE;

  static VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {
      VK_FALSE,                            // blendEnable
      VK_BLEND_FACTOR_SRC_ALPHA,           // srcColorBlendFactor
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // dstColorBlendFactor
      VK_BLEND_OP_ADD,                     // colorBlendOp
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // srcAlphaBlendFactor
      VK_BLEND_FACTOR_ZERO,                // dstAlphaBlendFactor
      VK_BLEND_OP_ADD,                     // alphaBlendOp
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, // colorWriteMask
  };

  params.color_blend_state = (VkPipelineColorBlendStateCreateInfo){
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      NULL,
      0,                          // flags
      VK_FALSE,                   // logicOpEnable
      VK_LOGIC_OP_COPY,           // logicOp
      1,                          // attachmentCount
      &colorBlendAttachmentState, // pAttachments
      {0.0f, 0.0f, 0.0f, 0.0f},   // blendConstants
  };

  return params;
}

re_pipeline_parameters_t eg_gizmo_pipeline_parameters() {
  re_pipeline_parameters_t params = re_default_pipeline_parameters();

  params.rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;
  params.rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

  return params;
}
