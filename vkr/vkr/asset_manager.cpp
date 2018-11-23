#include "asset_manager.hpp"

using namespace vkr;

void AssetManager::destroy() {
  for (auto &texture : textures) {
    if (texture) {
      texture.destroy();
    }
  }

  for (auto &model : gltfModels) {
    if (model) {
      model.destroy();
    }
  }
}

template <>
const Texture &AssetManager::getAsset<Texture>(const std::string &identifier) {
  if (assetTable.find(identifier) != assetTable.end()) {
    auto &asset = this->assetTable.at(identifier);

    if (asset.type == AssetType::eTexture) {
      return this->textures[asset.index];
    } else {
      throw std::runtime_error("Wrong asset type");
    }
  }

  fstl::log::debug("Loading texture asset: {}", identifier);

  Texture texture;
  texture.loadFromPath(identifier);

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
const GltfModel &
AssetManager::getAsset<GltfModel>(const std::string &identifier) {
  if (assetTable.find(identifier) != assetTable.end()) {
    auto &asset = this->assetTable.at(identifier);

    if (asset.type == AssetType::eGltfModel) {
      return this->gltfModels[asset.index];
    } else {
      throw std::runtime_error("Wrong asset type");
    }
  }

  fstl::log::debug("Loading glTF model asset: {}", identifier);

  // TODO: fix loading models with flipped UVs

  GltfModel model;
  model.loadFromPath(identifier);

  // Allocate GltfModel
  size_t modelIndex = gltfModels.size();

  if (loadedGltfModelCount < gltfModels.size()) {
    for (size_t i = 0; i < gltfModels.size(); i++) {
      if (!gltfModels[i]) {
        modelIndex = i;
        break;
      }
    }
  }

  if (modelIndex == gltfModels.size()) {
    this->gltfModels.push_back(model);
  } else {
    this->gltfModels[modelIndex] = model;
  }

  this->loadedGltfModelCount++;

  this->assetTable[identifier] = Asset{modelIndex, AssetType::eGltfModel};

  return gltfModels[modelIndex];
}

template <>
void AssetManager::unloadAsset<Texture>(const std::string &identifier) {
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

template <>
void AssetManager::unloadAsset<GltfModel>(const std::string &identifier) {
  if (assetTable.find(identifier) == assetTable.end()) {
    return;
  }

  auto &asset = this->assetTable.at(identifier);

  if (asset.type == AssetType::eGltfModel) {
    this->gltfModels[asset.index].destroy();
    assetTable.erase(identifier);
  } else {
    throw std::runtime_error("Wrong asset type");
  }
}
