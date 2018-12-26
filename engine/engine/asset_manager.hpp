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
    // auto id = AssetTypeId::type<A>;
    ensureAssetType<A>();
    return (A &)*m_assets[assetIndex];
  }

  template <typename A, typename... Args> A &loadAsset(Args... args) {
    auto id = getAssetType<A>();
    ensureAssetType<A>();

    AssetIndex assetIndex = -1;

    if (m_assets.size() == 0) {
      assetIndex = 0;
    } else {
      for (AssetIndex i = 0; i < m_assets.size(); i++) {
        assetIndex = i;
        if (!m_assets[i]) {
          break;
        }
      }
    }

    this->ensureAssetIndex<A>(assetIndex);

    return this->loadAssetIntoIndex<A>(assetIndex, args...);
  }

  template <typename A, typename... Args>
  A &loadAssetIntoIndex(AssetIndex assetIndex, Args... args) {
    auto id = getAssetType<A>();
    ensureAssetType<A>();

    if (!this->ensureAssetIndex<A>(assetIndex)) {
      throw std::runtime_error("Asset index is in use");
    }

    m_assets[assetIndex] = new A(std::forward<Args>(args)...);

    m_assets[assetIndex]->m_assetType = id;
    m_assets[assetIndex]->m_assetIndex = assetIndex;

    return (A &)*m_assets[assetIndex];
  }

  template <typename A> void unloadAsset(const AssetIndex assetIndex) {
    fstl::log::debug("Unloading asset #{}", assetIndex);

    auto id = getAssetType<A>();
    ensureAssetType<A>();

    m_assetDestructors[id](m_assets[assetIndex]);
    m_assets[assetIndex] = nullptr;
  }

  void each(std::function<void(Asset *)> callback) {
    for (AssetIndex i = 0; i < m_assets.size(); i++) {
      if (m_assets[i]) {
        callback(m_assets[i]);
      }
    }
  }

  template <typename A> static constexpr AssetType getAssetType() {
    const AssetType id = AssetTypeId::type<A>;
    return id;
  }

private:
  // Indexed by asset index
  std::vector<Asset *> m_assets;
  // Indexed by asset type
  std::vector<std::function<void(Asset *)>> m_assetDestructors;

  template <typename A> void ensureAssetType() {
    auto id = getAssetType<A>();

    if (m_assetDestructors.size() <= id) {
      m_assetDestructors.resize(id + 1);
    }
    if (!m_assetDestructors[id]) {
      m_assetDestructors[id] = [](Asset *asset) { delete ((A *)asset); };
    }
  }

  template <typename A> bool ensureAssetIndex(AssetIndex assetIndex) {
    // auto id = getAssetType<A>();

    if (m_assets.size() > assetIndex) {
      return !m_assets[assetIndex];
    } else {
      while (m_assets.size() <= assetIndex) {
        if (m_assets.size() == 0) {
          // Initial asset allocation
          m_assets.resize(4);
        } else {
          m_assets.resize(m_assets.size() * 2);
        }
      }
    }
    return true;
  }
};
} // namespace engine
