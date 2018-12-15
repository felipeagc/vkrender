#pragma once

#include "gltf_model.hpp"
#include <atomic>
#include <fstl/logging.hpp>
#include <renderer/texture.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace engine {

const size_t MAX_ASSET_TYPES = 20;

namespace detail {
template <typename...> class TypeId {
  inline static std::atomic<std::size_t> identifier;

  template <typename...>
  inline static const auto inner = identifier.fetch_add(1);

public:
  template <typename... Type>
  inline static const std::size_t type = inner<std::decay_t<Type>...>;
};
} // namespace detail

class AssetManager {
  using AssetTypeId = detail::TypeId<struct AssetManagerDummy>;

public:
  struct AssetInfo {
    size_t assetIndex;
    size_t assetTypeId;
  };

  AssetManager(){};
  ~AssetManager();

  // No copying AssetManager
  AssetManager(const AssetManager &) = delete;
  AssetManager &operator=(const AssetManager &) = delete;

  const std::unordered_map<std::string, AssetInfo> &getAssetTable() const;

  template <typename Asset, typename... Args>
  Asset getAsset(const std::string &path, Args... args) {
    this->ensure<Asset>();

    auto id = AssetTypeId::type<Asset>;

    assert(sizeof(Asset) == m_assetSizes[id]);

    if (m_assetTable.find(path) != m_assetTable.end()) {
      size_t assetIndex = m_assetTable[path].assetIndex;
      return *((Asset *)&m_assets[id][assetIndex * sizeof(Asset)]);
    }

    size_t assetIndex = 0;

    for (size_t i = 0; i < m_assetsInUse[id].size(); i++) {
      assetIndex = i + 1;
      if (!m_assetsInUse[id][i]) {
        break;
      }
    }

    if (m_assetsInUse[id].size() <= assetIndex) {
      m_assetsInUse[id].resize(assetIndex + 1);
      // TODO: resize changes the addresses of stuff
      // (Temporary) solution: just don't return addresses or references!
      // Return copies instead.
      m_assets[id].resize((assetIndex + 1) * sizeof(Asset));
      assetIndex = m_assetsInUse[id].size() - 1;
    }

    fstl::log::debug("Loading asset: {}, at index {}", path, assetIndex);

    Asset *asset = new (&m_assets[id][assetIndex * sizeof(Asset)])
        Asset{path, std::forward<Args>(args)...};

    m_assetsInUse[id][assetIndex] = true;

    m_assetTable[path] = AssetInfo{assetIndex, id};

    return *asset;
  }

  template <typename Asset> void unloadAsset(const std::string &path) {
    this->ensure<Asset>();
    if (m_assetTable.find(path) == m_assetTable.end()) {
      throw std::runtime_error("Tried to unload unloaded asset");
    }

    fstl::log::debug("Unloading asset: {}", path);

    auto id = AssetTypeId::type<Asset>;

    assert(sizeof(Asset) == m_assetSizes[id]);

    auto assetIndex = m_assetTable[path].assetIndex;
    m_assetDestructors[id]((void *)&m_assets[id][assetIndex * sizeof(Asset)]);

    m_assetTable.erase(path);

    m_assetsInUse[id][assetIndex] = false;
  }

private:
  std::array<std::vector<uint8_t>, MAX_ASSET_TYPES> m_assets;
  std::array<std::vector<bool>, MAX_ASSET_TYPES> m_assetsInUse;
  std::array<std::function<void(void *)>, MAX_ASSET_TYPES> m_assetDestructors;
  std::array<size_t, MAX_ASSET_TYPES> m_assetSizes;

  std::unordered_map<std::string, AssetInfo> m_assetTable;

  /*
    Ensures that a component has all of its information initialized.
   */
  template <typename Asset> void ensure() {
    auto id = AssetTypeId::type<Asset>;
    if (!m_assetDestructors[id]) {
      m_assetDestructors[id] = [](void *x) {
        static_cast<Asset *>(x)->destroy();
      };
    }

    m_assetSizes[id] = sizeof(Asset);

    // TODO: error when id is MAX_ASSET_TYPES
  }
};
} // namespace engine
