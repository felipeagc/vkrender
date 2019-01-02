#include "texture_asset.hpp"
#include "../scene.hpp"

using namespace engine;

template <>
void engine::loadAsset<TextureAsset>(
    const scene::Asset &asset, AssetManager &assetManager) {
  const std::string &path = asset.properties.at("path").getString();

  auto start = timeBegin(fmt::format("Loading Texture: {}", path));

  assetManager.loadAssetIntoIndex<TextureAsset>(asset.id, path);

  timeEnd(start, fmt::format("Finished loading: {}", path));
}
