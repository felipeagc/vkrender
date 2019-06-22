#pragma once

#include <renderer/pipeline.h>

// Common parameters:
// - blend enable (bool)
// - depth testing (bool)
// - cull mode (enum)
// - front face(enum)
// - polygon mode (enum)
// - line width (float)
// - vertex type

typedef enum {
  EG_CULL_MODE_BACK = 0,
  EG_CULL_MODE_FRONT,
  EG_CULL_MODE_NONE,
  EG_CULL_MODE_MAX,
} eg_cull_mode_t;

typedef enum {
  EG_FRONT_FACE_COUNTER_CLOCKWISE = 0,
  EG_FRONT_FACE_CLOCKWISE,
  EG_FRONT_FACE_MAX,
} eg_front_face_t;

typedef enum {
  EG_POLYGON_MODE_FILL = 0,
  EG_POLYGON_MODE_LINE,
  EG_POLYGON_MODE_MAX,
} eg_polygon_mode_t;

typedef enum {
  EG_VERTEX_TYPE_DEFAULT = 0,
  EG_VERTEX_TYPE_NONE,
  EG_VERTEX_TYPE_IMGUI,
  EG_VERTEX_TYPE_MAX,
} eg_vertex_type_t;

typedef struct eg_pipeline_params_t {
  bool blend;
  bool depth_test;
  bool depth_write;
  eg_cull_mode_t cull_mode;
  eg_front_face_t front_face;
  eg_polygon_mode_t polygon_mode;
  float line_width;
  eg_vertex_type_t vertex_type;
} eg_pipeline_params_t;

void eg_init_pipeline_spv(
    re_pipeline_t *pipeline,
    const char *paths[],
    uint32_t path_count,
    const eg_pipeline_params_t params);

eg_pipeline_params_t eg_default_pipeline_params();

eg_pipeline_params_t eg_imgui_pipeline_params();

eg_pipeline_params_t eg_billboard_pipeline_params();

eg_pipeline_params_t eg_outline_pipeline_params();

eg_pipeline_params_t eg_skybox_pipeline_params();

eg_pipeline_params_t eg_fullscreen_pipeline_params();
