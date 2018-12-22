#pragma once

#include <atomic>
#include <fstl/logging.hpp>
#include <functional>
#include <renderer/texture.hpp>
#include <string>
#include <vector>

namespace engine {

using AssetIndex = size_t;
using AssetType = size_t;

const AssetType MAX_ASSET_TYPES = 20;

class Asset {
  friend class AssetManager;

public:
  Asset(){};

  inline AssetIndex type() const noexcept { return m_assetType; }
  inline AssetIndex index() const noexcept { return m_assetIndex; }
  inline const char *identifier() { return m_identifier; }

protected:
  AssetType m_assetType = -1;
  AssetIndex m_assetIndex = -1;
  char m_identifier[128] = "";
};

namespace detail {
template <typename...> class TypeId {
  inline static std::atomic<AssetType> identifier;

  template <typename...>
  inline static const auto inner = identifier.fetch_add(1);

public:
  template <typename... Type>
  inline static const AssetType type = inner<std::decay_t<Type>...>;
};
} // namespace detail

class AssetManager {
  using AssetTypeId = detail::TypeId<struct AssetManagerDummy>;

public:
  AssetManager(){};
  ~AssetManager();

  // No copying AssetManager
  AssetManager(const AssetManager &) = delete;
  AssetManager &operator=(const AssetManager &) = delete;

  template <typename A> A &getAsset(const AssetIndex assetIndex) {
    auto id = AssetTypeId::type<A>;
    assert(sizeof(A) == m_assetSizes[id]);
    assert(m_assets[id].size() > assetIndex * sizeof(A));
    return *((A *)&m_assets[id][assetIndex * sizeof(A)]);
  }

  template <typename A, typename... Args> A &loadAsset(Args... args) {
    this->ensure<A>();

    auto id = getAssetType<A>();

    assert(sizeof(A) == m_assetSizes[id]);

    // if (m_assetTable.find(path) != m_assetTable.end()) {
    //   AssetIndex assetIndex = m_assetTable[path].assetIndex;
    //   return *((A*)&m_assets[id][assetIndex * sizeof(A)]);
    // }

    AssetIndex assetIndex = 0;

    for (AssetIndex i = 0; i < m_assetsInUse[id].size(); i++) {
      assetIndex = i + 1;
      if (!m_assetsInUse[id][i]) {
        break;
      }
    }

    if (m_assetsInUse[id].size() <= assetIndex) {
      m_assetsInUse[id].resize(assetIndex + 1);
      // IMPORTANT: resize changes the addresses of stuff
      // Never store asset references
      m_assets[id].resize((assetIndex + 1) * sizeof(A));
      assetIndex = m_assetsInUse[id].size() - 1;
    }

    // fstl::log::debug("Loading asset: {}, at index {}", path, assetIndex);

    A *asset = new (&m_assets[id][assetIndex * sizeof(A)])
        A{std::forward<Args>(args)...};
    asset->m_assetType = id;
    asset->m_assetIndex = assetIndex;

    m_assetsInUse[id][assetIndex] = true;

    return *asset;
  }

  template <typename A> void unloadAsset(const AssetIndex assetIndex) {
    fstl::log::debug("Unloading asset #{}", assetIndex);

    this->ensure<A>();
    auto id = getAssetType<A>();

    assert(sizeof(A) == m_assetSizes[id]);

    m_assetDestructors[id]((void *)&m_assets[id][assetIndex * sizeof(A)]);

    m_assetsInUse[id][assetIndex] = false;
  }

  void each(std::function<void(Asset *)> callback) {
    for (AssetType id = 0; id < MAX_ASSET_TYPES; id++) {
      for (AssetIndex i = 0; i < m_assetsInUse[id].size(); i++) {
        if (m_assetsInUse[id][i]) {
          Asset *asset = (Asset *)&m_assets[id][i * m_assetSizes[id]];
          callback(asset);
        }
      }
    }
  }

  template <typename A> static constexpr AssetType getAssetType() {
    const AssetType id = AssetTypeId::type<A>;
    return id;
  }

private:
  std::array<std::vector<uint8_t>, MAX_ASSET_TYPES> m_assets;
  std::array<std::vector<bool>, MAX_ASSET_TYPES> m_assetsInUse;
  std::array<std::function<void(void *)>, MAX_ASSET_TYPES> m_assetDestructors;
  std::array<size_t, MAX_ASSET_TYPES> m_assetSizes;

  /*
    Ensures that a component has all of its information initialized.
   */
  template <typename A> void ensure() {
    auto id = getAssetType<A>();
    if (!m_assetDestructors[id]) {
      m_assetDestructors[id] = [](void *x) { static_cast<A *>(x)->~A(); };
    }

    m_assetSizes[id] = sizeof(A);

    // TODO: error when id is MAX_ASSET_TYPES
  }
};
} // namespace engine
