#include "asset_manager.hpp"

using namespace engine;

AssetManager::~AssetManager() {
  for (auto &texture : m_textures) {
    if (texture) {
      texture.destroy();
    }
  }

  for (auto &model : m_gltfModels) {
    if (model) {
      model.destroy();
    }
  }
}

std::unordered_map<std::string, Asset> &AssetManager::getAssetTable() {
  return m_assetTable;
}

template <>
const renderer::Texture &
AssetManager::getAsset<renderer::Texture>(const std::string &identifier) {
  if (m_assetTable.find(identifier) != m_assetTable.end()) {
    auto &asset = m_assetTable.at(identifier);

    if (asset.type == AssetType::eTexture) {
      return m_textures[asset.index];
    } else {
      throw std::runtime_error("Wrong asset type");
    }
  }

  fstl::log::debug("Loading texture asset: {}", identifier);

  renderer::Texture texture{identifier};

  // Allocate Texture
  size_t textureIndex = m_textures.size();

  if (m_loadedTextureCount < m_textures.size()) {
    for (size_t i = 0; i < m_textures.size(); i++) {
      if (!m_textures[i]) {
        textureIndex = i;
        break;
      }
    }
  }

  if (textureIndex == m_textures.size()) {
    m_textures.push_back(texture);
  } else {
    m_textures[textureIndex] = texture;
  }

  m_loadedTextureCount++;

  m_assetTable[identifier] = Asset{textureIndex, AssetType::eTexture};

  return m_textures[textureIndex];
}

template <>
const GltfModel &
AssetManager::getAsset<GltfModel>(const std::string &identifier) {
  if (m_assetTable.find(identifier) != m_assetTable.end()) {
    auto &asset = m_assetTable.at(identifier);

    if (asset.type == AssetType::eGltfModel) {
      return m_gltfModels[asset.index];
    } else {
      throw std::runtime_error("Wrong asset type");
    }
  }

  fstl::log::debug("Loading glTF model asset: {}", identifier);

  GltfModel model{identifier};

  // Allocate GltfModel
  size_t modelIndex = m_gltfModels.size();

  if (m_loadedGltfModelCount < m_gltfModels.size()) {
    for (size_t i = 0; i < m_gltfModels.size(); i++) {
      if (!m_gltfModels[i]) {
        modelIndex = i;
        break;
      }
    }
  }

  if (modelIndex == m_gltfModels.size()) {
    m_gltfModels.push_back(model);
  } else {
    m_gltfModels[modelIndex] = model;
  }

  m_loadedGltfModelCount++;

  m_assetTable[identifier] = Asset{modelIndex, AssetType::eGltfModel};

  return m_gltfModels[modelIndex];
}

template <>
void AssetManager::unloadAsset<renderer::Texture>(
    const std::string &identifier) {
  if (m_assetTable.find(identifier) == m_assetTable.end()) {
    return;
  }

  auto &asset = m_assetTable.at(identifier);

  if (asset.type == AssetType::eTexture) {
    m_textures[asset.index].destroy();
    m_assetTable.erase(identifier);
  } else {
    throw std::runtime_error("Wrong asset type");
  }
}

template <>
void AssetManager::unloadAsset<GltfModel>(const std::string &identifier) {
  if (m_assetTable.find(identifier) == m_assetTable.end()) {
    return;
  }

  auto &asset = m_assetTable.at(identifier);

  if (asset.type == AssetType::eGltfModel) {
    m_gltfModels[asset.index].destroy();
    m_assetTable.erase(identifier);
  } else {
    throw std::runtime_error("Wrong asset type");
  }
}
