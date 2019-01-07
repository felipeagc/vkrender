#include "scene.hpp"
#include "assets/environment_asset.hpp"
#include "assets/gltf_model_asset.hpp"
#include "assets/texture_asset.hpp"
#include "components/billboard_component.hpp"
#include "components/camera_component.hpp"
#include "components/environment_component.hpp"
#include "components/gltf_model_component.hpp"
#include "components/light_component.hpp"
#include "components/name_component.hpp"
#include "components/transform_component.hpp"
#include <renderer/context.hpp>
#include <renderer/thread_pool.hpp>

namespace engine {

static inline void
loadAssets(const sdf::SceneFile &scene, AssetManager &assetManager) {
  renderer::ThreadPool threadPool(renderer::VKR_THREAD_COUNT);
  for (auto &asset : scene.assets) {
    renderer::Job job;
    if (strcmp(asset.type, "GltfModel") == 0) {
      job = [&]() { loadAsset<GltfModelAsset>(asset, assetManager); };
    } else if (strcmp(asset.type, "Texture") == 0) {
      job = [&]() { loadAsset<TextureAsset>(asset, assetManager); };
    } else if (strcmp(asset.type, "Environment") == 0) {
      job = [&]() { loadAsset<EnvironmentAsset>(asset, assetManager); };
    } else {
      ftl::warn("Unsupported asset type: %s", asset.type);
    }

    if (job) {
      threadPool.addJob(job);
    }
  }

  threadPool.wait();
}

static inline void loadEntities(
    const sdf::SceneFile &scene,
    ecs::World &world,
    AssetManager &assetManager) {
  for (auto &entity : scene.entities) {
    ecs::Entity e = world.createEntity();

    world.assign<NameComponent>(e, entity.name);

    for (auto &comp : entity.components) {
      if (strcmp(comp.name, "GltfModel") == 0) {
        loadComponent<GltfModelComponent>(comp, world, assetManager, e);
      } else if (strcmp(comp.name, "Transform") == 0) {
        loadComponent<TransformComponent>(comp, world, assetManager, e);
      } else if (strcmp(comp.name, "Light") == 0) {
        loadComponent<LightComponent>(comp, world, assetManager, e);
      } else if (strcmp(comp.name, "Billboard") == 0) {
        loadComponent<BillboardComponent>(comp, world, assetManager, e);
      } else if (strcmp(comp.name, "Environment") == 0) {
        loadComponent<EnvironmentComponent>(comp, world, assetManager, e);
      } else if (strcmp(comp.name, "Camera") == 0) {
        loadComponent<CameraComponent>(comp, world, assetManager, e);
      } else {
        ftl::warn("Unsupported component type: %s", comp.name);
      }
    }
  }
}

Scene::Scene(const std::string &path) {
  FILE *file = fopen(path.c_str(), "r");

  if (file != nullptr) {
    auto result = sdf::parse_file(file);
    fclose(file);

    if (!result) {
      throw std::runtime_error(
          "Failed to parse scene: " + result.error.message);
    }

    loadAssets(result.value, m_assetManager);
    loadEntities(result.value, m_world, m_assetManager);
  } else {
    throw std::runtime_error("Failed to open scene file");
  }
}

} // namespace engine
