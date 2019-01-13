#include "imgui_utils.hpp"
#include "asset_manager.hpp"
#include "assets/environment_asset.hpp"
#include "assets/gltf_model_asset.hpp"
#include "assets/texture_asset.hpp"
#include "components/camera_component.hpp"
#include "components/environment_component.hpp"
#include "components/gltf_model_component.hpp"
#include "components/light_component.hpp"
#include "components/transform_component.hpp"
#include <imgui/imgui.h>
#include <renderer/context.hpp>
#include <renderer/util.hpp>
#include <renderer/window.hpp>

namespace engine {
namespace imgui {
void statsWindow(const re_window_t *window) {
  ImGui::Begin("Stats");

  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(g_ctx.physical_device, &properties);

  ImGui::Text("Physical device: %s", properties.deviceName);
  ImGui::Text("Delta time: %.3f s", window->delta_time);
  ImGui::Text("Framerate: %.2f", 1.0f / window->delta_time);

  ImGui::End();
}

void assetsWindow(AssetManager &assetManager) {
  ImGui::Begin("Assets");

  auto &assets = assetManager.getAssets();
  for (size_t assetIndex = 0; assetIndex < assets.size(); assetIndex++) {
    Asset *asset = assets[assetIndex];
    if (asset == nullptr) {
      continue;
    }

    ImGui::PushID(assetIndex);

    char str[128] = "";

    if (asset->type == AssetManager::getAssetType<GltfModelAsset>()) {
      sprintf(
          str, "GLTF Model #%lu: %s", assetIndex, asset->identifier.c_str());

      if (ImGui::CollapsingHeader(str)) {
        auto *gltfModelAsset = (GltfModelAsset *)asset;

        for (size_t i = 0; i < gltfModelAsset->m_materials.size(); i++) {
          ImGui::PushID(i);
          ImGui::Indent();

          auto &mat = gltfModelAsset->m_materials[i];

          sprintf(str, "Material #%lu", i);

          if (ImGui::CollapsingHeader(str)) {
            ImGui::ColorEdit4("Base color factor", &mat.ubo.baseColorFactor.x);
            ImGui::SliderFloat("Metallic", &mat.ubo.metallic, 0.0f, 1.0f);
            ImGui::SliderFloat("Roughness", &mat.ubo.roughness, 0.0f, 1.0f);
          }

          ImGui::Unindent();
          ImGui::PopID();
        }
      }
    } else if (asset->type == AssetManager::getAssetType<EnvironmentAsset>()) {
      sprintf(
          str, "Environment #%lu: %s", assetIndex, asset->identifier.c_str());

      if (ImGui::CollapsingHeader(str)) {
        // auto *environmentAsset = (EnvironmentAsset *)asset;
        ImGui::Text("Environment asset");
      }
    } else if (asset->type == AssetManager::getAssetType<TextureAsset>()) {
      sprintf(str, "Texture #%lu: %s", assetIndex, asset->identifier.c_str());

      if (ImGui::CollapsingHeader(str)) {
        // auto *textureAsset = (TextureAsset *)asset;
        ImGui::Text("Texture asset");
      }
    }

    ImGui::PopID();
  }

  ImGui::End();
}
} // namespace imgui
} // namespace engine
