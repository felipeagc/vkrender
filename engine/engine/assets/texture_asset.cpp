#include "texture_asset.hpp"
#include "../scene.hpp"

using namespace engine;

template <>
void engine::loadAsset<TextureAsset>(
    const sdf::AssetBlock &asset, AssetManager &assetManager) {
  std::string path;

  for (auto &prop : asset.properties) {
    if (strcmp(prop.name, "path") == 0) {
      prop.get_string(path);
    }
  }

  char str[128] = "";
  sprintf(str, "Loading Texture: %s", path.c_str());
  auto start = timeBegin(str);

  auto &a = assetManager.loadAssetIntoIndex<TextureAsset>(asset.index, path);
  a.identifier = asset.name;

  sprintf(str, "Finished loading: %s", path.c_str());
  timeEnd(start, str);
}
