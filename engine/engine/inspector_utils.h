#pragma once

#include "asset_manager.h"
#include "imgui.h"
#include "inspector.h"

typedef struct {
  char *label;
  eg_asset_t **asset;
  eg_asset_type_t type;
} eg_inspect_assets_t;

static inline void eg_inspect_assets(
    eg_inspector_t *inspector,
    void *comp,
    uint32_t asset_count,
    eg_inspect_assets_t *assets) {
  igPushIDPtr(comp);
  igColumns(3, NULL, true);

  for (uint32_t i = 0; i < asset_count; i++) {
    const char *label    = assets[i].label;
    eg_asset_t **asset   = assets[i].asset;
    eg_asset_type_t type = assets[i].type;

    igPushIDStr(label);

    igText(label);
    igNextColumn();

    const char *name =
        (*asset != NULL) ? eg_asset_get_name(*asset) : "No asset";

    if (igSelectable(name, false, 0, (ImVec2){0})) {
      igOpenPopup("selectasset");
    }

    if (igBeginPopup("selectasset", 0)) {
      for (uint32_t i = 0; i < inspector->asset_manager->max_index; i++) {
        eg_asset_t *cur_asset =
            eg_asset_manager_get(inspector->asset_manager, i);
        if (cur_asset == NULL) continue;
        if (cur_asset->type != type) continue;

        if (igSelectable(eg_asset_get_name(cur_asset), false, 0, (ImVec2){0})) {
          *asset = cur_asset;
        }
      }
      igEndPopup();
    }

    igNextColumn();

    if (*asset) {
      assert((*asset)->type == type);

      if (igSelectable("Inspect", false, 0, (ImVec2){0})) {
        igOpenPopup("inspectasset");
      }

      if (igBeginPopup("inspectasset", 0)) {
        EG_ASSET_INSPECTORS[(*asset)->type](*asset, inspector);
        igEndPopup();
      }

      igSameLine(0.0f, -1.0f);
    }

    igNextColumn();

    igPopID();
  }

  igColumns(1, NULL, true);
  igPopID();
}
