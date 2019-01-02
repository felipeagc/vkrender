#pragma once

#include "asset_manager.hpp"
#include <chrono>
#include <ecs/world.hpp>
#include <fstl/logging.hpp>
#include <scene/scene.hpp>
#include <string>

namespace engine {

inline std::chrono::time_point<std::chrono::system_clock>
timeBegin(const std::string_view &msg) {
  fstl::log::debug("{}", msg);
  return std::chrono::system_clock::now();
}
inline void timeEnd(
    std::chrono::time_point<std::chrono::system_clock> start,
    const std::string_view &msg) {
  auto end = std::chrono::system_clock::now();
  float seconds =
      (float)std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
          .count() /
      1000.f;
  fstl::log::debug("[{:.2f}s] {}", seconds, msg);
}

template <typename A> void loadAsset(const scene::Asset &, AssetManager &) {}

template <typename C>
void loadComponent(
    const scene::Component &, ecs::World &, AssetManager &, ecs::Entity) {}

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
