#include "renderable_comp.h"

#include "../assets/pipeline_asset.h"
#include "../inspector_utils.h"
#include "../serializer.h"

void eg_renderable_comp_default(eg_renderable_comp_t *renderable) {
  renderable->pipeline = NULL;
}

void eg_renderable_comp_inspect(
    eg_renderable_comp_t *renderable, eg_inspector_t *inspector) {
  eg_inspect_assets(
      inspector,
      renderable,
      1,
      (eg_inspect_assets_t[]){
          {"Pipeline",
           (eg_asset_t **)&renderable->pipeline,
           EG_ASSET_TYPE(eg_pipeline_asset_t)},
      });
}

void eg_renderable_comp_destroy(eg_renderable_comp_t *renderable) {
  renderable->pipeline = NULL;
}

enum {
  PROP_PIPELINE,
  PROP_MAX,
};

void eg_renderable_comp_serialize(
    eg_renderable_comp_t *renderable, eg_serializer_t *serializer) {
  eg_serializer_append_u32(serializer, PROP_MAX);

  eg_asset_uid_t uid = EG_NULL_ASSET_UID;
  if (renderable->pipeline) uid = renderable->pipeline->asset.uid;

  eg_serializer_append_u32(serializer, PROP_PIPELINE);
  eg_serializer_append(serializer, &uid, sizeof(uid));
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
