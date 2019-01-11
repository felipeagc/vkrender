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

    // @todo: remove unnecessary allocations
    const char **paths = new const char *[radiancePaths.size()];
    for (uint32_t i = 0; i < radiancePaths.size(); i++) {
      paths[i] = radiancePaths[i].c_str();
    }

    re_cubemap_init_from_hdr_equirec(
        &m_skyboxCubemap, skyboxPath.c_str(), width, height);
    re_cubemap_init_from_hdr_equirec(
        &m_irradianceCubemap, irradiancePath.c_str(), width, height);
    re_cubemap_init_from_hdr_equirec_mipmaps(
        &m_radianceCubemap, paths, radiancePaths.size(), width, height);

    delete[] paths;

    re_texture_init_from_path(&m_brdfLut, brdfLutPath.c_str());
  }

  ~EnvironmentAsset() {
    re_cubemap_destroy(&m_skyboxCubemap);
    re_cubemap_destroy(&m_irradianceCubemap);
    re_cubemap_destroy(&m_radianceCubemap);

    re_texture_destroy(&m_brdfLut);
  }

  // EnvironmentAsset cannot be copied
  EnvironmentAsset(const EnvironmentAsset &) = delete;
  EnvironmentAsset &operator=(const EnvironmentAsset &) = delete;

  // EnvironmentAsset cannot be moved
  EnvironmentAsset(EnvironmentAsset &&) = delete;
  EnvironmentAsset &operator=(EnvironmentAsset &&) = delete;

  re_cubemap_t m_skyboxCubemap;
  re_cubemap_t m_irradianceCubemap;
  re_cubemap_t m_radianceCubemap;
  re_texture_t m_brdfLut;
};
} // namespace engine
