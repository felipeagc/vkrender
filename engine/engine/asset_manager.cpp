#include "asset_manager.hpp"

using namespace engine;

AssetManager::AssetManager() { bump_allocator_init(&m_allocator, 16384); }

AssetManager::~AssetManager() {
  std::scoped_lock<std::mutex> lock(m_mutex);
  for (size_t assetId = 0; assetId < m_assets.size(); assetId++) {
    if (m_assets[assetId] != nullptr) {
      Asset *asset = m_assets[assetId];
      m_assetDestructors[asset->type](asset);
    }
  }

  bump_allocator_destroy(&m_allocator);
}
