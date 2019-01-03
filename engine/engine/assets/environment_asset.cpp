#include "environment_asset.hpp"
#include "../scene.hpp"

using namespace engine;

template <>
void engine::loadAsset<EnvironmentAsset>(
    const scene::Asset &asset, AssetManager &assetManager) {
  const uint32_t width =
      static_cast<uint32_t>(asset.properties.at("size").values[0].getInt());
  const uint32_t height =
      static_cast<uint32_t>(asset.properties.at("size").values[1].getInt());
  const std::string &skybox = asset.properties.at("skybox").getString();
  const std::string &irradiance = asset.properties.at("irradiance").getString();
  std::vector<std::string> radiance(
      asset.properties.at("radiance").values.size());
  for (size_t i = 0; i < asset.properties.at("radiance").values.size(); i++) {
    radiance[i] = asset.properties.at("radiance").values[i].getString();
  }
  const std::string &brdfLut = asset.properties.at("brdfLut").getString();

  char str[128] = "";
  sprintf(str, "Loading Environment: %s", skybox.c_str());
  auto start = timeBegin(str);

  assetManager.loadAssetIntoIndex<EnvironmentAsset>(
      asset.id, width, height, skybox, irradiance, radiance, brdfLut);

  sprintf(str, "Finished loading: %s", skybox.c_str());
  timeEnd(start, str);
}
