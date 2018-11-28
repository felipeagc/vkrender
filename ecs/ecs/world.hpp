#pragma once

#include "entity.hpp"
#include "typeid.hpp"
#include <bitset>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <vector>

namespace ecs {

const size_t MAX_COMPONENTS = 100;
const size_t INITIAL_ALLOCATED_ENTITY_COUNT = 1000;

using ComponentMask = std::bitset<MAX_COMPONENTS>;

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
  template<typename Component>
  bool hasComponent(Entity entity) {
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
    const size_t index = entity * sizeof(Component);

    this->ensure<Component>();

    if (m_entityComponentMasks.size() <= entity) {
      throw std::runtime_error("Invalid entity");
    }

    auto compId = WorldTypeId::type<Component>;

    std::vector<std::uint8_t> &memory = m_componentVectors[compId];

    if (memory.size() <= index) {
      memory.resize((entity + 1) * sizeof(Component));
    }

    if (m_entityComponentMasks[entity][compId]) {
      // Entity already has component
      // So let's destroy it
      Component *comp = (Component *)&memory[index];
      comp->~Component();
    }

    m_entityComponentMasks[entity].set(compId);

    new (&memory[index]) Component{std::forward<Args>(args)...};
  }

  /*
    Removes a component from an entity and destroys the component.
   */
  template <typename Component> void removeComponent(Entity entity) {
    auto compId = WorldTypeId::type<Component>;

    // If entity has the component
    if (m_entityComponentMasks[entity][compId]) {
      std::vector<std::uint8_t> &memory = m_componentVectors[compId];
      const size_t index = entity * sizeof(Component);

      // Remove the component
      Component *comp = (Component *)&memory[index];
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

    return (Component *)&m_componentVectors[compId][entity * sizeof(Component)];
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

protected:
  std::vector<std::vector<uint8_t>> m_componentVectors;
  std::array<std::function<void(const void *)>, MAX_COMPONENTS>
      m_componentDestructors;
  std::array<size_t, MAX_COMPONENTS> m_componentSizes;
  std::vector<ComponentMask> m_entityComponentMasks;

  /*
    Ensures that a component has all of its information initialized.
   */
  template <typename Component> void ensure() {
    auto id = WorldTypeId::type<Component>;
    if (!m_componentDestructors[id]) {
      m_componentDestructors[id] = [](const void *x) {
        static_cast<const Component *>(x)->~Component();
      };
    }

    if (m_componentSizes[id] == 0) {
      m_componentSizes[id] = sizeof(Component);
    }

    // TODO: these allocations might get expensive
    if (m_componentVectors.size() <= id) {
      m_componentVectors.resize(id + 1);
    }
  }
};
} // namespace ecs
