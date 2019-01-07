#pragma once

#include <atomic>
#include <ftl/logging.hpp>
#include <functional>
#include <mutex>
#include <renderer/texture.hpp>
#include <string>
#include <thread>
#include <vector>

namespace engine {

using AssetIndex = size_t;
using AssetType = size_t;

const AssetType MAX_ASSET_TYPES = 20;

struct Asset {
  AssetType type = -1;
  AssetIndex index = -1;
  std::string identifier = "";
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

  const std::vector<Asset *> &getAssets() { return m_assets; }

  template <typename A> A &getAsset(const AssetIndex assetIndex) {
    ensureAssetType<A>();
    return (A &)*m_assets[assetIndex];
  }

  template <typename A, typename... Args> A &loadAsset(Args... args) {
    ensureAssetType<A>();

    AssetIndex assetIndex = SIZE_MAX;
    {
      std::scoped_lock<std::mutex> lock(m_mutex);

      if (m_assets.size() == 0) {
        assetIndex = 0;
      } else {
        for (AssetIndex i = 0; i < m_assets.size(); i++) {
          if (!m_assets[i]) {
            assetIndex = i;
            break;
          }
        }
      }

      if (assetIndex == SIZE_MAX) {
        assetIndex = m_assets.size();
      }
    }

    this->ensureAssetIndex<A>(assetIndex);

    return this->loadAssetIntoIndex<A>(assetIndex, args...);
  }

  template <typename A, typename... Args>
  A &loadAssetIntoIndex(AssetIndex assetIndex, Args... args) {
    auto id = getAssetType<A>();
    ensureAssetType<A>();

    // ftl::debug("Loading asset #%lu", assetIndex);

    if (!this->ensureAssetIndex<A>(assetIndex)) {
      throw std::runtime_error(
          "Asset index is in use: " + std::to_string(assetIndex));
    }

    A *asset = new A(std::forward<Args>(args)...);

    std::scoped_lock<std::mutex> lock(m_mutex);

    m_assets[assetIndex] = asset;

    m_assets[assetIndex]->type = id;
    m_assets[assetIndex]->index = assetIndex;

    return (A &)*m_assets[assetIndex];
  }

  template <typename A> void unloadAsset(const AssetIndex assetIndex) {
    ftl::debug("Unloading asset #%lu", assetIndex);

    auto id = getAssetType<A>();
    ensureAssetType<A>();

    m_assetDestructors[id](m_assets[assetIndex]);
    m_assets[assetIndex] = nullptr;
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

  std::mutex m_mutex;

  template <typename A> void ensureAssetType() {
    auto id = getAssetType<A>();

    std::scoped_lock<std::mutex> lock(m_mutex);

    if (m_assetDestructors.size() <= id) {
      m_assetDestructors.resize(id + 1);
    }
    if (!m_assetDestructors[id]) {
      m_assetDestructors[id] = [](Asset *asset) { delete ((A *)asset); };
    }
  }

  template <typename A> bool ensureAssetIndex(AssetIndex assetIndex) {
    std::scoped_lock<std::mutex> lock(m_mutex);

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
