#include <ecs/world.hpp>
#include <engine/engine.hpp>
#include <fstl/fixed_vector.hpp>
#include <fstl/logging.hpp>
#include <imgui/imgui.h>
#include <renderer/renderer.hpp>
#include <scene/driver.hpp>

int main() {
  renderer::Context context;
  renderer::Window window("GLTF models", 800, 600, VK_SAMPLE_COUNT_1_BIT);

  window.clearColor = {0.15, 0.15, 0.15, 1.0};

  engine::AssetManager assetManager;

  ecs::World world;
  engine::GltfModelSystem gltfModelSystem;
  engine::FPSCameraSystem fpsCameraSystem;
  engine::BillboardSystem billboardSystem;
  engine::SkyboxSystem skyboxSystem;
  engine::LightingSystem lightingSystem;
  engine::EntityInspectorSystem entityInspectorSystem;

  // Create shaders & pipelines
  renderer::Shader skyboxShader{
      "../shaders/skybox.vert",
      "../shaders/skybox.frag",
  };

  renderer::GraphicsPipeline skyboxPipeline =
      renderer::SkyboxPipeline(window, skyboxShader);

  skyboxShader.destroy();

  engine::ShaderWatcher<renderer::StandardPipeline> modelShaderWatcher(
      window, "../shaders/model_pbr.vert", "../shaders/model_pbr.frag");

  engine::ShaderWatcher<renderer::StandardPipeline> billboardShaderWatcher(
      window, "../shaders/billboard.vert", "../shaders/billboard.frag");

  // Load scene
  scene::Driver drv;
  FILE *file = fopen("../assets/main.scene", "r");
  if (file) {
    drv.parseFile("", file);
    fclose(file);
  } else {
    throw std::runtime_error("Failed to open scene file");
  }

  for (auto &asset : drv.m_scene.assets) {
    if (asset.type == "GltfModel") {
      const std::string &path = asset.properties["path"].getString();
      bool flipUVs = false;
      if (asset.properties.find("flip_uvs") != asset.properties.end()) {
        flipUVs = true;
      }

      assetManager.loadAssetIntoIndex<engine::GltfModelAsset>(asset.id, path, flipUVs);
    } else if (asset.type == "Texture") {
      const std::string &path = asset.properties["path"].getString();
      assetManager.loadAssetIntoIndex<engine::TextureAsset>(asset.id, path);
    } else if (asset.type == "Environment") {
      const uint32_t width =
          static_cast<uint32_t>(asset.properties["size"].values[0].getInt());
      const uint32_t height =
          static_cast<uint32_t>(asset.properties["size"].values[1].getInt());
      const std::string &skybox = asset.properties["skybox"].getString();
      const std::string &irradiance =
          asset.properties["irradiance"].getString();
      std::vector<std::string> radiance(
          asset.properties["radiance"].values.size());
      for (size_t i = 0; i < asset.properties["radiance"].values.size(); i++) {
        radiance[i] = asset.properties["radiance"].values[i].getString();
      }
      const std::string &brdfLut = asset.properties["brdfLut"].getString();

      assetManager.loadAssetIntoIndex<engine::EnvironmentAsset>(
          asset.id, width, height, skybox, irradiance, radiance, brdfLut);
    } else {
      fstl::log::warn("Unsupported asset type: {}", asset.type);
    }
  }

  for (auto &entity : drv.m_scene.entities) {
    ecs::Entity e = world.createEntity();

    if (entity.components.find("GltfModel") != entity.components.end()) {
      auto &comp = entity.components["GltfModel"];
      world.assign<engine::GltfModelComponent>(
          e,
          assetManager.getAsset<engine::GltfModelAsset>(
              comp.properties["asset"].getUint32()));
    }

    if (entity.components.find("Transform") != entity.components.end()) {
      auto &comp = entity.components["Transform"];

      glm::vec3 pos(0.0);
      glm::vec3 scale(1.0);
      glm::quat rotation{1.0, 0.0, 0.0, 0.0};

      if (comp.properties.find("position") != comp.properties.end()) {
        pos = comp.properties["position"].getVec3();
      }

      if (comp.properties.find("scale") != comp.properties.end()) {
        scale = comp.properties["scale"].getVec3();
      }

      if (comp.properties.find("rotation") != comp.properties.end()) {
        rotation = comp.properties["rotation"].getQuat();
      }

      world.assign<engine::TransformComponent>(e, pos, scale, rotation);
    }

    if (entity.components.find("Light") != entity.components.end()) {
      auto &comp = entity.components["Light"];
      glm::vec3 color(1.0);
      float intensity = 1.0f;

      if (comp.properties.find("color") != comp.properties.end()) {
        color = comp.properties["color"].getVec3();
      }

      if (comp.properties.find("intensity") != comp.properties.end()) {
        intensity = comp.properties["intensity"].getFloat();
      }

      world.assign<engine::LightComponent>(e, color, intensity);
    }

    if (entity.components.find("Billboard") != entity.components.end()) {
      auto &comp = entity.components["Billboard"];
      world.assign<engine::BillboardComponent>(
          e,
          assetManager.getAsset<engine::TextureAsset>(
              comp.properties["asset"].getUint32()));
    }

    if (entity.components.find("Environment") != entity.components.end()) {
      auto &comp = entity.components["Environment"];
      world.assign<engine::EnvironmentComponent>(
          e,
          assetManager.getAsset<engine::EnvironmentAsset>(
              comp.properties["asset"].getUint32()));
    }

    if (entity.components.find("Camera") != entity.components.end()) {
      world.assign<engine::CameraComponent>(e);
    }
  }

  float time = 0.0;

  modelShaderWatcher.startWatching();
  billboardShaderWatcher.startWatching();

  while (!window.getShouldClose()) {
    window.beginPresent();

    time += window.getDelta();

    SDL_Event event;
    while (window.pollEvent(&event)) {
      fpsCameraSystem.processEvent(window, event);

      switch (event.type) {
      case SDL_QUIT:
        fstl::log::info("Goodbye");
        window.setShouldClose(true);
        break;
      }
    }

    // Show ImGui windows
    engine::imgui::statsWindow(window);
    engine::imgui::assetsWindow(assetManager);

    modelShaderWatcher.lockPipeline();
    billboardShaderWatcher.lockPipeline();

    entityInspectorSystem.process(world);
    lightingSystem.process(window, world);
    fpsCameraSystem.process(window, world);
    gltfModelSystem.process(
        window, assetManager, world, modelShaderWatcher.pipeline());
    skyboxSystem.process(window, world, skyboxPipeline);
    billboardSystem.process(window, world, billboardShaderWatcher.pipeline());

    window.endPresent();
  }

  return 0;
}
