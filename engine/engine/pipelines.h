#pragma once

#include <renderer/pipeline.h>

void eg_init_pipeline_spv(
    re_pipeline_t *pipeline,
    const re_render_target_t *render_target,
    const char *vertex_path,
    const char *fragment_path,
    const re_pipeline_parameters_t params);

typedef struct eg_default_pipeline_layouts_t {
  re_pipeline_layout_t pbr;
  re_pipeline_layout_t skybox;
} eg_default_pipeline_layouts_t;

extern eg_default_pipeline_layouts_t g_default_pipeline_layouts;

void eg_default_pipeline_layouts_init();

void eg_default_pipeline_layouts_destroy();

re_pipeline_parameters_t eg_standard_pipeline_parameters();

re_pipeline_parameters_t eg_picking_pipeline_parameters();

re_pipeline_parameters_t eg_billboard_pipeline_parameters();

re_pipeline_parameters_t eg_outline_pipeline_parameters();

re_pipeline_parameters_t eg_skybox_pipeline_parameters();

re_pipeline_parameters_t eg_fullscreen_pipeline_parameters();

re_pipeline_parameters_t eg_gizmo_pipeline_parameters();
