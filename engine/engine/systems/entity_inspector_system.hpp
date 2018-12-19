#pragma once

#include <ecs/world.hpp>

namespace engine {
class EntityInspectorSystem {
public:
  EntityInspectorSystem();
  ~EntityInspectorSystem();

  void process(ecs::World &world);
};
} // namespace engine
