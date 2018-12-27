#pragma once

#include "asset_manager.hpp"
#include <ecs/world.hpp>
#include <scene/scene.hpp>
#include <string>

namespace engine {

template <typename A> void loadAsset(const scene::Asset &, AssetManager &){};

template <typename C>
void loadComponent(
    const scene::Component &, ecs::World &, AssetManager &, ecs::Entity){};

class Scene {
public:
  Scene(const std::string &path);
  ~Scene(){};

  // Scene cannot be copied
  Scene(const Scene &other) = delete;
  Scene &operator=(const Scene &other) = delete;

  ecs::World m_world;
  AssetManager m_assetManager;
};
} // namespace engine
