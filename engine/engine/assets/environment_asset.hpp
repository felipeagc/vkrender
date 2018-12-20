#pragma once

#include "../asset_manager.hpp"
#include <renderer/cubemap.hpp>

namespace engine {
class EnvironmentAsset : public Asset {
public:
  EnvironmentAsset() {}

  EnvironmentAsset(
      const uint32_t width,
      const uint32_t height,
      const std::string &skyboxPath,
      const std::string &irradiancePath,
      const std::vector<std::string> &radiancePaths,
      const std::string &brdfLutPath) {
    strncpy(m_identifier, skyboxPath.c_str(), sizeof(m_identifier));
    m_skyboxCubemap = renderer::Cubemap(skyboxPath, width, height);
    m_irradianceCubemap = renderer::Cubemap(irradiancePath, width, height);
    m_radianceCubemap = renderer::Cubemap(radiancePaths, width, height);
    m_brdfLut = renderer::Texture(brdfLutPath);
  }

  ~EnvironmentAsset() {
    m_skyboxCubemap.destroy();
    m_irradianceCubemap.destroy();
    m_radianceCubemap.destroy();
    m_brdfLut.destroy();
  }

  // EnvironmentAsset cannot be copied
  EnvironmentAsset(const EnvironmentAsset &) = delete;
  EnvironmentAsset &operator=(const EnvironmentAsset &) = delete;

  // EnvironmentAsset cannot be moved
  EnvironmentAsset(EnvironmentAsset &&) = delete;
  EnvironmentAsset &operator=(EnvironmentAsset &&) = delete;

  const renderer::Cubemap &skyboxCubemap() const { return m_skyboxCubemap; }
  const renderer::Cubemap &irradianceCubemap() const {
    return m_irradianceCubemap;
  }
  const renderer::Cubemap &radianceCubemap() const { return m_radianceCubemap; }
  const renderer::Texture &brdfLut() const { return m_brdfLut; }

private:
  renderer::Cubemap m_skyboxCubemap;
  renderer::Cubemap m_irradianceCubemap;
  renderer::Cubemap m_radianceCubemap;
  renderer::Texture m_brdfLut;
};
} // namespace engine
