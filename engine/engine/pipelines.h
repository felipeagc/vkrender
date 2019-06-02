#pragma once

#include <renderer/pipeline.h>

void eg_init_pipeline_spv(
    re_pipeline_t *pipeline,
    const re_render_target_t *render_target,
    const char *paths[],
    uint32_t path_count,
    const re_pipeline_parameters_t params);

re_pipeline_parameters_t eg_standard_pipeline_parameters();

re_pipeline_parameters_t eg_picking_pipeline_parameters();

re_pipeline_parameters_t eg_billboard_pipeline_parameters();

re_pipeline_parameters_t eg_outline_pipeline_parameters();

re_pipeline_parameters_t eg_skybox_pipeline_parameters();

re_pipeline_parameters_t eg_fullscreen_pipeline_parameters();

re_pipeline_parameters_t eg_gizmo_pipeline_parameters();
