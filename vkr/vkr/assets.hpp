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

  template <typename A> A &getAsset(const std::string &);
  template <> Texture &getAsset<Texture>(const std::string &identifier);

  template <typename A> void destroyAsset(const std::string &);
  template <> void destroyAsset<Texture>(const std::string &identifier);

  void destroy();
};
} // namespace vkr
