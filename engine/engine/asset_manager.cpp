#include "asset_manager.hpp"

using namespace engine;

AssetManager::~AssetManager() {
  for (size_t typeId = 0; typeId < MAX_ASSET_TYPES; typeId++) {
    for (size_t assetId = 0; assetId < m_assetsInUse[typeId].size();
         assetId++) {
      if (m_assetsInUse[typeId][assetId]) {
        void *asset = &m_assets[typeId][assetId * m_assetSizes[typeId]];
        m_assetDestructors[typeId](asset);
      }
    }
  }
}
