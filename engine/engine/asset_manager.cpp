#include "asset_manager.hpp"

using namespace engine;

AssetManager::~AssetManager() {
  for (size_t id = 0; id < MAX_ASSET_TYPES; id++) {
    for (size_t i = 0; i < m_assetsInUse[id].size(); i++) {
      if (m_assetsInUse[id][i]) {
        std::string assetPath;

        void *asset = &m_assets[id][i * m_assetSizes[id]];

        for (auto [path, ptr] : m_assetTable) {
          if (ptr == asset) {
            assetPath = path;
            break;
          }
        }

        fstl::log::debug("Unloading asset: {}", assetPath);

        m_assetDestructors[id](asset);
      }
    }
  }
}

const std::unordered_map<std::string, void *> &
AssetManager::getAssetTable() const {
  return m_assetTable;
}
