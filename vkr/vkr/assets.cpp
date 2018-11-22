#include "assets.hpp"

using namespace vkr;

template <>
Texture &AssetManager::getAsset<Texture>(const std::string &identifier) {
  if (assetTable.find(identifier) != assetTable.end()) {
    auto &asset = this->assetTable.at(identifier);

    if (asset.type == AssetType::eTexture) {
      return this->textures[asset.index];
    } else {
      throw std::runtime_error("Wrong asset type");
    }
  }

  fstl::log::debug("Loading texture: " + identifier);

  Texture texture(identifier);
  // Allocate Texture
  size_t textureIndex = textures.size();

  if (loadedTextureCount < textures.size()) {
    for (size_t i = 0; i < textures.size(); i++) {
      if (!textures[i]) {
        textureIndex = i;
        break;
      }
    }
  }

  if (textureIndex == textures.size()) {
    this->textures.push_back(texture);
  } else {
    this->textures[textureIndex] = texture;
  }

  this->loadedTextureCount++;

  this->assetTable[identifier] = Asset{textureIndex, AssetType::eTexture};

  return textures[textureIndex];
}

template <>
void AssetManager::destroyAsset<Texture>(const std::string &identifier) {
  if (assetTable.find(identifier) == assetTable.end()) {
    return;
  }

  auto &asset = this->assetTable.at(identifier);

  if (asset.type == AssetType::eTexture) {
    this->textures[asset.index].destroy();
    assetTable.erase(identifier);
  } else {
    throw std::runtime_error("Wrong asset type");
  }
}

void AssetManager::destroy() {
  for (auto &texture : textures) {
    if (texture) {
      texture.destroy();
    }
  }

  assetTable.clear();
}
