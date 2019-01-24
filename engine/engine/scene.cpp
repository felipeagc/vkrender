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
#include <util/task_scheduler.hpp>

namespace engine {

struct asset_load_bundle_t {
  sdf::AssetBlock const *asset;
  AssetManager *assetManager;
};

void *load_gltf_model(void *args) {
  asset_load_bundle_t *bundle = (asset_load_bundle_t *)args;
  loadAsset<GltfModelAsset>(*bundle->asset, *bundle->assetManager);

  free(bundle);
  return NULL;
}

void *load_environment(void *args) {
  asset_load_bundle_t *bundle = (asset_load_bundle_t *)args;
  loadAsset<EnvironmentAsset>(*bundle->asset, *bundle->assetManager);

  free(bundle);
  return NULL;
}

void *load_texture(void *args) {
  asset_load_bundle_t *bundle = (asset_load_bundle_t *)args;
  loadAsset<TextureAsset>(*bundle->asset, *bundle->assetManager);

  free(bundle);
  return NULL;
}

static inline void
loadAssets(const sdf::SceneFile &scene, AssetManager &assetManager) {
  ut_task_scheduler scheduler;
  ut_scheduler_init(&scheduler, RE_THREAD_COUNT);

  for (auto &asset : scene.assets) {
    asset_load_bundle_t *bundle =
        (asset_load_bundle_t *)malloc(sizeof(asset_load_bundle_t));
    bundle->asset = &asset;
    bundle->assetManager = &assetManager;
    if (strcmp(asset.type, "GltfModel") == 0) {
      ut_scheduler_add_task(&scheduler, load_gltf_model, bundle);
    } else if (strcmp(asset.type, "Texture") == 0) {
      ut_scheduler_add_task(&scheduler, load_texture, bundle);
    } else if (strcmp(asset.type, "Environment") == 0) {
      ut_scheduler_add_task(&scheduler, load_environment, bundle);
    } else {
      ftl::warn("Unsupported asset type: %s", asset.type);
    }
  }

  ut_scheduler_destroy(&scheduler);
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
