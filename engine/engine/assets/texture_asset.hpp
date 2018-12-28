#pragma once

#include "../asset_manager.hpp"
#include <fstl/logging.hpp>
#include <renderer/texture.hpp>

namespace engine {
class TextureAsset : public Asset {
public:
  TextureAsset() {}

  TextureAsset(const std::string &path) {
    m_identifier = path;
    fstl::log::debug("Loading Texture asset: \"{}\"", m_identifier);
    m_texture = renderer::Texture(path);
  }

  TextureAsset(
      const std::vector<unsigned char> &data,
      const uint32_t width,
      const uint32_t height) {
    m_texture = renderer::Texture(data, width, height);
  }

  ~TextureAsset() { m_texture.destroy(); }

  // TextureAsset cannot be copied
  TextureAsset(const TextureAsset &) = delete;
  TextureAsset &operator=(const TextureAsset &) = delete;

  // TextureAsset cannot be moved
  TextureAsset(TextureAsset &&) = delete;
  TextureAsset &operator=(TextureAsset &&) = delete;

  const renderer::Texture &texture() const { return m_texture; }

private:
  renderer::Texture m_texture;
};
} // namespace engine
