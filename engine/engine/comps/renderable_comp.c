#include "renderable_comp.h"

#include "../assets/pipeline_asset.h"
#include "../imgui.h"

void eg_renderable_comp_default(eg_renderable_comp_t *renderable) {
  renderable->pipeline = NULL;
}

void eg_renderable_comp_inspect(
    eg_renderable_comp_t *renderable, eg_inspector_t *inspector) {
  if (renderable->pipeline) {
    igText("Pipeline asset: %s", renderable->pipeline->asset.name);

    igSameLine(0.0f, -1.0f);
    if (igSmallButton("Inspect")) {
      igOpenPopup("renderablepopup");
    }

    if (igBeginPopup("renderablepopup", 0)) {
      eg_pipeline_asset_inspect(renderable->pipeline, inspector);
      igEndPopup();
    }
  }
}

void eg_renderable_comp_destroy(eg_renderable_comp_t *renderable) {
  renderable->pipeline = NULL;
}

void eg_renderable_comp_init(
    eg_renderable_comp_t *renderable, eg_pipeline_asset_t *pipeline) {
  renderable->pipeline = pipeline;
}

re_pipeline_t *
eg_renderable_comp_get_pipeline(eg_renderable_comp_t *renderable) {
  if (renderable->pipeline == NULL) return NULL;
  return &renderable->pipeline->pipeline;
}
