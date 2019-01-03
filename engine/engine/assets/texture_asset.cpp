#include "texture_asset.hpp"
#include "../scene.hpp"

using namespace engine;

template <>
void engine::loadAsset<TextureAsset>(
    const scene::Asset &asset, AssetManager &assetManager) {
  const std::string &path = asset.properties.at("path").getString();

  char str[128] = "";
  sprintf(str, "Loading Texture: %s", path.c_str());
  auto start = timeBegin(str);

  assetManager.loadAssetIntoIndex<TextureAsset>(asset.id, path);

  sprintf(str, "Finished loading: %s", path.c_str());
  timeEnd(start, str);
}
