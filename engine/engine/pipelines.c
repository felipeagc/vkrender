#include "pipelines.h"

#include "filesystem.h"
#include <fstd_util.h>
#include <renderer/context.h>
#include <renderer/util.h>
#include <renderer/window.h>

static re_pipeline_parameters_t
convert_params(const eg_pipeline_params_t *params) {
  re_pipeline_parameters_t re_params = re_default_pipeline_parameters();

  re_params.rasterization_state.lineWidth = params->line_width;

  switch (params->cull_mode) {
  case EG_CULL_MODE_NONE: {
    re_params.rasterization_state.cullMode = VK_CULL_MODE_NONE;
    break;
  }
  case EG_CULL_MODE_BACK: {
    re_params.rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;
    break;
  }
  case EG_CULL_MODE_FRONT: {
    re_params.rasterization_state.cullMode = VK_CULL_MODE_FRONT_BIT;
    break;
  }
  default: break;
  }

  switch (params->front_face) {
  case EG_FRONT_FACE_CLOCKWISE: {
    re_params.rasterization_state.frontFace = VK_FRONT_FACE_CLOCKWISE;
    break;
  }
  case EG_FRONT_FACE_COUNTER_CLOCKWISE: {
    re_params.rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    break;
  }
  default: break;
  }

  switch (params->polygon_mode) {
  case EG_POLYGON_MODE_FILL: {
    re_params.rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
    break;
  }
  case EG_POLYGON_MODE_LINE: {
    re_params.rasterization_state.polygonMode = VK_POLYGON_MODE_LINE;
    break;
  }
  default: break;
  }

  static VkPipelineColorBlendAttachmentState
      color_blend_attachment_state_enabled = {
          .blendEnable         = VK_TRUE,
          .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
          .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
          .colorBlendOp        = VK_BLEND_OP_ADD,
          .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
          .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
          .alphaBlendOp        = VK_BLEND_OP_ADD,
          .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT |
                            VK_COLOR_COMPONENT_G_BIT |

                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,

      };

  static VkPipelineColorBlendAttachmentState
      color_blend_attachment_state_disabled = {
          .blendEnable         = VK_FALSE,
          .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
          .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
          .colorBlendOp        = VK_BLEND_OP_ADD,
          .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
          .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
          .alphaBlendOp        = VK_BLEND_OP_ADD,
          .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT |
                            VK_COLOR_COMPONENT_G_BIT |

                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,

      };

  re_params.color_blend_state.attachmentCount = 1;
  if (params->blend) {
    re_params.color_blend_state.pAttachments =
        &color_blend_attachment_state_enabled;
  } else {
    re_params.color_blend_state.pAttachments =
        &color_blend_attachment_state_disabled;
  }

  re_params.depth_stencil_state.depthTestEnable = (VkBool32)params->depth_test;
  re_params.depth_stencil_state.depthWriteEnable =
      (VkBool32)params->depth_write;

  switch (params->vertex_type) {
  case EG_VERTEX_TYPE_NONE: {
    re_params.vertex_input_state.vertexBindingDescriptionCount   = 0;
    re_params.vertex_input_state.pVertexBindingDescriptions      = NULL;
    re_params.vertex_input_state.vertexAttributeDescriptionCount = 0;
    re_params.vertex_input_state.pVertexAttributeDescriptions    = NULL;
    break;
  }
  case EG_VERTEX_TYPE_DEFAULT: {
    static VkVertexInputBindingDescription vertex_binding_descriptions[] = {
        {.binding   = 0,
         .stride    = sizeof(re_vertex_t),
         .inputRate = VK_VERTEX_INPUT_RATE_VERTEX}};

    static VkVertexInputAttributeDescription vertex_attribute_descriptions[] = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(re_vertex_t, pos)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(re_vertex_t, normal)},
        {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(re_vertex_t, uv)}};

    re_params.vertex_input_state.vertexBindingDescriptionCount =
        ARRAY_SIZE(vertex_binding_descriptions);
    re_params.vertex_input_state.pVertexBindingDescriptions =
        vertex_binding_descriptions;
    re_params.vertex_input_state.vertexAttributeDescriptionCount =
        ARRAY_SIZE(vertex_attribute_descriptions);
    re_params.vertex_input_state.pVertexAttributeDescriptions =
        vertex_attribute_descriptions;
    break;
  }
  case EG_VERTEX_TYPE_IMGUI: {
    static VkVertexInputBindingDescription binding_desc[1] = {{
        .binding   = 0,
        .stride    = sizeof(float) * 2 + sizeof(float) * 2 + sizeof(uint32_t),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    }};

    static VkVertexInputAttributeDescription attribute_desc[3] = {
        {.location = 0,
         .binding  = 0,
         .format   = VK_FORMAT_R32G32_SFLOAT,
         .offset   = 0},
        {.location = 1,
         .binding  = 0,
         .format   = VK_FORMAT_R32G32_SFLOAT,
         .offset   = sizeof(float) * 2},
        {.location = 2,
         .binding  = 0,
         .format   = VK_FORMAT_R8G8B8A8_UNORM,
         .offset   = sizeof(float) * 4}};

    re_params.vertex_input_state.vertexBindingDescriptionCount =
        ARRAY_SIZE(binding_desc);
    re_params.vertex_input_state.pVertexBindingDescriptions = binding_desc;
    re_params.vertex_input_state.vertexAttributeDescriptionCount =
        ARRAY_SIZE(attribute_desc);
    re_params.vertex_input_state.pVertexAttributeDescriptions = attribute_desc;
    break;
  }
  default: break;
  }

  return re_params;
}

void eg_init_pipeline_spv(
    re_pipeline_t *pipeline,
    const char *paths[],
    uint32_t path_count,
    const eg_pipeline_params_t params) {
  uint8_t *codes[RE_MAX_SHADER_STAGES] = {0};
  re_shader_t shaders[RE_MAX_SHADER_STAGES];

  for (uint32_t i = 0; i < path_count; i++) {
    eg_file_t *file = eg_file_open_read(paths[i]);
    assert(file);
    size_t size = eg_file_size(file);
    codes[i]    = calloc(1, size);
    eg_file_read_bytes(file, codes[i], size);
    eg_file_close(file);

    re_shader_init_spv(&shaders[i], (uint32_t *)codes[i], size);
  }

  re_pipeline_init_graphics(
      pipeline, shaders, path_count, convert_params(&params));

  for (uint32_t i = 0; i < path_count; i++) {
    free(codes[i]);
  }
}

eg_pipeline_params_t eg_default_pipeline_params() {
  return (eg_pipeline_params_t){
      .blend        = true,
      .depth_test   = true,
      .depth_write  = true,
      .cull_mode    = EG_CULL_MODE_BACK,
      .front_face   = EG_FRONT_FACE_COUNTER_CLOCKWISE,
      .polygon_mode = EG_POLYGON_MODE_FILL,
      .line_width   = 1.0f,
      .vertex_type  = EG_VERTEX_TYPE_DEFAULT,
  };
}

eg_pipeline_params_t eg_imgui_pipeline_params() {
  eg_pipeline_params_t params = eg_default_pipeline_params();

  params.vertex_type = EG_VERTEX_TYPE_IMGUI;
  params.depth_test  = false;
  params.depth_write = false;
  params.cull_mode   = EG_CULL_MODE_NONE;

  return params;
}

eg_pipeline_params_t eg_billboard_pipeline_params() {
  eg_pipeline_params_t params = eg_default_pipeline_params();

  params.vertex_type = EG_VERTEX_TYPE_NONE;

  return params;
}

eg_pipeline_params_t eg_outline_pipeline_params() {
  eg_pipeline_params_t params = eg_default_pipeline_params();

  params.cull_mode    = EG_CULL_MODE_NONE;
  params.polygon_mode = EG_POLYGON_MODE_LINE;
  params.line_width   = 2.0f;

  return params;
}

eg_pipeline_params_t eg_skybox_pipeline_params() {
  eg_pipeline_params_t params = eg_default_pipeline_params();

  params.depth_write = false;
  params.cull_mode   = EG_CULL_MODE_NONE;
  params.vertex_type = EG_VERTEX_TYPE_NONE;

  return params;
}

eg_pipeline_params_t eg_fullscreen_pipeline_params() {
  eg_pipeline_params_t params = eg_default_pipeline_params();

  params.blend       = false;
  params.depth_test  = false;
  params.cull_mode   = EG_CULL_MODE_FRONT;
  params.vertex_type = EG_VERTEX_TYPE_NONE;

  return params;
}
