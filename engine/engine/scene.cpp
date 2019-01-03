#include "scene.hpp"
#include "assets/environment_asset.hpp"
#include "assets/gltf_model_asset.hpp"
#include "assets/texture_asset.hpp"
#include "components/billboard_component.hpp"
#include "components/camera_component.hpp"
#include "components/environment_component.hpp"
#include "components/gltf_model_component.hpp"
#include "components/light_component.hpp"
#include "components/transform_component.hpp"
#include <renderer/context.hpp>
#include <renderer/thread_pool.hpp>
#include <scene/driver.hpp>
#include <scene/scene.hpp>

namespace engine {

static inline void
loadAssets(const scene::Scene &scene, AssetManager &assetManager) {
  renderer::ThreadPool threadPool(renderer::VKR_THREAD_COUNT);
  for (auto &asset : scene.assets) {
    renderer::Job job;
    if (asset.type == "GltfModel") {
      job = [&]() { loadAsset<GltfModelAsset>(asset, assetManager); };
    } else if (asset.type == "Texture") {
      job = [&]() { loadAsset<TextureAsset>(asset, assetManager); };
    } else if (asset.type == "Environment") {
      job = [&]() { loadAsset<EnvironmentAsset>(asset, assetManager); };
    } else {
      ftl::warn("Unsupported asset type: %s", asset.type.c_str());
    }

    if (job) {
      threadPool.addJob(job);
    }
  }

  threadPool.wait();
}

static inline void loadEntities(
    const scene::Scene &scene, ecs::World &world, AssetManager &assetManager) {
  for (auto &entity : scene.entities) {
    ecs::Entity e = world.createEntity();

    for (auto &[compName, comp] : entity.components) {
      if (compName == "GltfModel") {
        loadComponent<GltfModelComponent>(comp, world, assetManager, e);
      } else if (compName == "Transform") {
        loadComponent<TransformComponent>(comp, world, assetManager, e);
      } else if (compName == "Light") {
        loadComponent<LightComponent>(comp, world, assetManager, e);
      } else if (compName == "Billboard") {
        loadComponent<BillboardComponent>(comp, world, assetManager, e);
      } else if (compName == "Environment") {
        loadComponent<EnvironmentComponent>(comp, world, assetManager, e);
      } else if (compName == "Camera") {
        loadComponent<CameraComponent>(comp, world, assetManager, e);
      } else {
        ftl::warn("Unsupported component type: %s", compName.c_str());
      }
    }
  }
}

Scene::Scene(const std::string &path) {
  scene::Driver drv;
  FILE *file = fopen(path.c_str(), "r");
  if (file != nullptr) {
    drv.parseFile(path, file);
    fclose(file);
  } else {
    throw std::runtime_error("Failed to open scene file");
  }

  loadAssets(drv.m_scene, m_assetManager);
  loadEntities(drv.m_scene, m_world, m_assetManager);
}

} // namespace engine
