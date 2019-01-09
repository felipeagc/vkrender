#pragma once

#include "entity.hpp"
#include "typeid.hpp"
#include <bitset>
#include <cassert>
#include <cstdint>
#include <ftl/vector.hpp>
#include <functional>
#include <stdexcept>

namespace ecs {

const size_t MAX_COMPONENTS = 50;
const size_t INITIAL_ALLOCATED_ENTITY_COUNT = 100;

using ComponentMask = std::bitset<MAX_COMPONENTS>;

struct ComponentAllocator {
  struct Block {
    Block() {}
    Block(size_t size) { this->storage = new uint8_t[size]; }

    uint8_t *storage = nullptr;
  };

  ComponentAllocator() {}

  ComponentAllocator(size_t componentSize, size_t componentsPerBlock = 4) {
    m_blockSize = componentSize * componentsPerBlock;
    m_componentSize = componentSize;
    m_componentsPerBlock = componentsPerBlock;
    m_initialized = true;
  }

  ~ComponentAllocator() {
    for (auto &block : m_blocks) {
      if (block.storage != nullptr) {
        delete[] block.storage;
      }
    }
  }

  void ensurePosition(size_t position) {
    size_t blockIndex = position / m_componentsPerBlock;

    if (blockIndex >= m_blocks.size()) {
      m_blocks.resize(blockIndex + 1);
    }

    if (m_blocks[blockIndex].storage == nullptr) {
      m_blocks[blockIndex] = Block(m_blockSize);
    }
  }

  uint8_t *operator[](size_t position) {
    size_t blockIndex = position / m_componentsPerBlock;
    size_t positionInBlock = position % m_componentsPerBlock;

    return &m_blocks[blockIndex].storage[positionInBlock * m_componentSize];
  }

  bool m_initialized = false;

private:
  ftl::small_vector<Block> m_blocks;
  size_t m_blockSize = 0;
  size_t m_componentSize = 0;
  size_t m_componentsPerBlock = 0;
};

struct ComponentInfo {
  ComponentAllocator allocator;
  size_t size;
  std::function<void(const void *)> destructor;
};

class World {
public:
  using WorldTypeId = TypeId<struct DummyWorldTypeId>;

  World();
  ~World();

  /*
   Gets the first available entity ID.
   If all of the slots are filled up, allocate some more.
   */
  Entity createEntity();

  /*
   Clears the entity component mask and its components
   and makes this entity ID available for use
   */
  void removeEntity(Entity entity);

  /*
    Returns whether the entity has at least one component associated with it.
   */
  bool hasComponents(Entity entity);

  /*
    Returns whether the entity has a type of component associated with it.
   */
  template <typename Component> bool hasComponent(Entity entity) {
    if (m_entityComponentMasks.size() <= entity) {
      throw std::runtime_error("Invalid entity");
    }

    auto compId = WorldTypeId::type<Component>;
    return m_entityComponentMasks[entity][compId];
  }

  /*
    Sets a component for an entity.
    If the entity already has this type of component,
    replace the old one with the new one.
   */
  template <typename Component, typename... Args>
  void assign(Entity entity, Args &&... args) {
    this->ensure<Component>();

    auto compId = WorldTypeId::type<Component>;

    assert(sizeof(Component) == m_componentInfos[compId].size);

    if (m_entityComponentMasks.size() <= entity) {
      throw std::runtime_error("Invalid entity");
    }

    auto &allocator = m_componentInfos[compId].allocator;

    allocator.ensurePosition(entity);

    if (m_entityComponentMasks[entity][compId]) {
      // Entity already has component
      // So let's destroy it
      Component *comp = (Component *)allocator[entity];
      comp->~Component();
    }

    m_entityComponentMasks[entity].set(compId);

    new (allocator[entity]) Component{std::forward<Args>(args)...};
  }

  /*
    Removes a component from an entity and destroys the component.
   */
  template <typename Component> void removeComponent(Entity entity) {
    auto compId = WorldTypeId::type<Component>;

    // If entity has the component
    if (m_entityComponentMasks[entity][compId]) {
      auto &allocator = m_componentInfos[compId].allocator;

      // Remove the component
      Component *comp = (Component *)allocator[entity];
      comp->~Component();
    }

    m_entityComponentMasks[entity].set(compId, 0);
  }

  /*
    Gets a specific component from an entity.
   */
  template <typename Component> Component *getComponent(Entity entity) {
    auto compId = WorldTypeId::type<Component>;

    if (!m_entityComponentMasks[entity][compId]) {
      return nullptr;
    }

    return (Component *)m_componentInfos[compId].allocator[entity];
  }

  template <typename C> ComponentMask componentMask() {
    ComponentMask mask;
    mask.set(WorldTypeId::type<C>);
    return mask;
  }

  template <typename C1, typename C2, typename... Components>
  ComponentMask componentMask() {
    return componentMask<C1>() | componentMask<C2, Components...>();
  }

  template <typename T> struct identity { typedef T type; };

  /*
    Iterates through all entities.
   */
  void each(typename identity<std::function<void(Entity)>>::type update) {
    for (Entity entity = 0; entity < m_entityComponentMasks.size(); entity++) {
      auto entityMask = m_entityComponentMasks[entity];
      if (!entityMask.none()) {
        update(entity);
      }
    }
  }

  /*
    Iterates through all entities with a component filter and the requested
    components as parameters.
   */
  template <typename... Components>
  void
  each(typename identity<std::function<void(Entity, Components &...)>>::type
           update) {
    auto mask = componentMask<Components...>();

    for (Entity entity = 0; entity < m_entityComponentMasks.size(); entity++) {
      auto entityMask = m_entityComponentMasks[entity];

      if ((mask & entityMask) == mask) {
        update(entity, (*this->getComponent<Components>(entity))...);
      }
    }
  }

  /*
    Returns the first entity with the required components.
   */
  template <typename... Components> Entity first() {
    auto mask = componentMask<Components...>();

    for (Entity entity = 0; entity < m_entityComponentMasks.size(); entity++) {
      auto entityMask = m_entityComponentMasks[entity];

      if ((mask & entityMask) == mask) {
        return entity;
      }
    }

    throw std::runtime_error("Couldn't find entity with required components.");
  }

protected:
  ftl::small_vector<ComponentInfo> m_componentInfos;
  ftl::small_vector<ComponentMask> m_entityComponentMasks;

  /*
    Ensures that a component has all of its information initialized.
   */
  template <typename Component> void ensure() {
    auto id = WorldTypeId::type<Component>;

    if (m_componentInfos.size() <= id) {
      m_componentInfos.resize(id + 1);
    }

    if (!m_componentInfos[id].allocator.m_initialized) {
      m_componentInfos[id].allocator = ComponentAllocator(sizeof(Component));
    }

    if (!m_componentInfos[id].destructor) {
      m_componentInfos[id].destructor = [](const void *x) {
        static_cast<const Component *>(x)->~Component();
      };
    }

    m_componentInfos[id].size = sizeof(Component);
  }
};
} // namespace ecs
