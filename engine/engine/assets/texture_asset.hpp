#pragma once

#include "../asset_manager.hpp"
#include <ftl/logging.hpp>
#include <renderer/texture.hpp>

namespace engine {
class TextureAsset : public Asset {
public:
  TextureAsset() {}

  TextureAsset(const std::string &path) {
    this->identifier = path;

    re_texture_init_from_path(&m_texture, path.c_str());
  }

  TextureAsset(
      const std::vector<unsigned char> &data,
      const uint32_t width,
      const uint32_t height) {
    re_texture_init(&m_texture, data.data(), data.size(), width, height);
  }

  ~TextureAsset() { re_texture_destroy(&m_texture); }

  // TextureAsset cannot be copied
  TextureAsset(const TextureAsset &) = delete;
  TextureAsset &operator=(const TextureAsset &) = delete;

  // TextureAsset cannot be moved
  TextureAsset(TextureAsset &&) = delete;
  TextureAsset &operator=(TextureAsset &&) = delete;

  re_texture_t m_texture;
};
} // namespace engine
