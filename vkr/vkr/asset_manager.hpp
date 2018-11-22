#pragma once

#include "texture.hpp"
#include <fstl/logging.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace vkr {
enum AssetType { eTexture };

struct Asset {
  size_t index = std::numeric_limits<size_t>::max();
  AssetType type;
};

struct AssetManager {
  size_t loadedTextureCount = 0;
  std::vector<Texture> textures;

  std::unordered_map<std::string, Asset> assetTable;

  template <typename H> const H &getAsset(const std::string &);
  template <>
  const Texture &
  getAsset<Texture>(const std::string &identifier);

  template <typename A> void unloadAsset(const std::string &);
  template <> void unloadAsset<Texture>(const std::string &identifier);

  void destroy();
};
} // namespace vkr
