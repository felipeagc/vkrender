#pragma once

#include "gltf_model.hpp"
#include "texture.hpp"
#include <fstl/logging.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace vkr {
enum AssetType { eTexture, eGltfModel };

struct Asset {
  size_t index = std::numeric_limits<size_t>::max();
  AssetType type;
};

class AssetManager {
public:
  AssetManager(){};
  // TODO: get rid of this and use a destructor
  void destroy();

  std::unordered_map<std::string, Asset> &getAssetTable();

  template <typename H> const H &getAsset(const std::string &);
  template <> const Texture &getAsset<Texture>(const std::string &identifier);
  template <>
  const GltfModel &getAsset<GltfModel>(const std::string &identifier);

  template <typename A> void unloadAsset(const std::string &);
  template <> void unloadAsset<Texture>(const std::string &identifier);
  template <> void unloadAsset<GltfModel>(const std::string &identifier);

private:
  size_t m_loadedTextureCount = 0;
  std::vector<Texture> m_textures;

  size_t m_loadedGltfModelCount = 0;
  std::vector<GltfModel> m_gltfModels;

  std::unordered_map<std::string, Asset> m_assetTable;
};
} // namespace vkr
