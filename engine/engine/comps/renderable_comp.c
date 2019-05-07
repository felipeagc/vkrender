#include "renderable_comp.h"

void eg_renderable_comp_init(
    eg_renderable_comp_t *renderable, eg_pipeline_asset_t *pipeline) {
  renderable->pipeline = pipeline;
}

void eg_renderable_comp_destroy(eg_renderable_comp_t *renderable) {
  renderable->pipeline = NULL;
}
