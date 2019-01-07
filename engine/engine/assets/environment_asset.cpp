#include "environment_asset.hpp"
#include "../scene.hpp"

using namespace engine;

template <>
void engine::loadAsset<EnvironmentAsset>(
    const sdf::AssetBlock &asset, AssetManager &assetManager) {
  uint32_t width = 1024;
  uint32_t height = 1024;
  std::string skybox;
  std::string irradiance;
  std::vector<std::string> radiance;
  std::string brdfLut;

  for (auto &prop : asset.properties) {
    if (strcmp(prop.name, "size") == 0) {
      if (prop.values.size() != 2) {
        throw std::runtime_error("Invalid size property for environment asset");
      }
      prop.values[0].get_uint32(&width);
      prop.values[1].get_uint32(&height);
    } else if (strcmp(prop.name, "skybox") == 0) {
      prop.get_string(skybox);
    } else if (strcmp(prop.name, "irradiance") == 0) {
      prop.get_string(irradiance);
    } else if (strcmp(prop.name, "radiance") == 0) {
      for (auto &value : prop.values) {
        std::string s;
        value.get_string(s);
        radiance.push_back(s);
      }
    } else if (strcmp(prop.name, "brdfLut") == 0) {
      prop.get_string(brdfLut);
    }
  }

  char str[128] = "";
  sprintf(str, "Loading Environment: %s", skybox.c_str());
  auto start = timeBegin(str);

  auto &a = assetManager.loadAssetIntoIndex<EnvironmentAsset>(
      asset.index, width, height, skybox, irradiance, radiance, brdfLut);
  a.identifier = asset.name;

  sprintf(str, "Finished loading: %s", skybox.c_str());
  timeEnd(start, str);
}
