#include "world.hpp"

using namespace ecs;

World::World() {
  m_entityComponentMasks.resize(INITIAL_ALLOCATED_ENTITY_COUNT);
}

World::~World() {
  for (Entity e = 0; e < m_entityComponentMasks.size(); e++) {
    this->removeEntity(e);
  }
}

Entity World::createEntity() {
  for (size_t i = 0; i < m_entityComponentMasks.size(); i++) {
    auto &bitset = m_entityComponentMasks[i];
    if (bitset.none()) {
      return i;
    }
  }

  size_t entityCount = m_entityComponentMasks.size();

  m_entityComponentMasks.resize(entityCount * 2);

  return entityCount;
}

void World::removeEntity(Entity entity) {
  if (m_entityComponentMasks.size() <= entity) {
    throw std::runtime_error("Invalid entity");
  }

  for (size_t compId = 0; compId < MAX_COMPONENTS; compId++) {
    if (m_entityComponentMasks[entity][compId]) {
      const void *component = m_componentInfos[compId].allocator[entity];
      m_componentInfos[compId].destructor(component);
      m_entityComponentMasks[entity][compId] = 0;
    }
  }

  // Resets component mask to zeroes
  this->m_entityComponentMasks[entity].reset();
}

bool World::hasComponents(Entity entity) {
  if (m_entityComponentMasks.size() <= entity) {
    throw std::runtime_error("Invalid entity");
  }

  return (!m_entityComponentMasks[entity].none());
}
