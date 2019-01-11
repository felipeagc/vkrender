#pragma once

#include "../asset_manager.hpp"
#include <ftl/logging.hpp>
#include <renderer/cubemap.hpp>
#include <renderer/texture.hpp>

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
    this->identifier = skyboxPath;

    m_skyboxCubemap = renderer::Cubemap(skyboxPath, width, height);
    m_irradianceCubemap = renderer::Cubemap(irradiancePath, width, height);
    m_radianceCubemap = renderer::Cubemap(radiancePaths, width, height);

    re_texture_init_from_path(&m_brdfLut, brdfLutPath.c_str());
  }

  ~EnvironmentAsset() {
    m_skyboxCubemap.destroy();
    m_irradianceCubemap.destroy();
    m_radianceCubemap.destroy();

    re_texture_destroy(&m_brdfLut);
  }

  // EnvironmentAsset cannot be copied
  EnvironmentAsset(const EnvironmentAsset &) = delete;
  EnvironmentAsset &operator=(const EnvironmentAsset &) = delete;

  // EnvironmentAsset cannot be moved
  EnvironmentAsset(EnvironmentAsset &&) = delete;
  EnvironmentAsset &operator=(EnvironmentAsset &&) = delete;

  renderer::Cubemap m_skyboxCubemap;
  renderer::Cubemap m_irradianceCubemap;
  renderer::Cubemap m_radianceCubemap;
  re_texture_t m_brdfLut;
};
} // namespace engine
