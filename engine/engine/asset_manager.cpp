#include "asset_manager.hpp"

using namespace engine;

AssetManager::~AssetManager() {
  for (size_t assetId = 0; assetId < m_assets.size(); assetId++) {
    if (m_assets[assetId] != nullptr) {
      Asset *asset = m_assets[assetId];
      m_assetDestructors[asset->type()](asset);
    }
  }
}
