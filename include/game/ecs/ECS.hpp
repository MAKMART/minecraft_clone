#pragma once
#include "entity_manager.hpp"
#include "component_manager.hpp"
#include <memory>

template<typename... Components>
class ECS {
public:
    ECS() = default;
    ~ECS() = default;

    // Create a new entity
    Entity createEntity() {
        return em.create();
    }

    // Destroy entity and all its components
    void destroyEntity(Entity e) {
        em.destroy(e);
        cm.remove_entity(e);
    }

    // Add component to entity
    template<typename T>
    void addComponent(Entity e, const T& component) {
        cm.template add_component<T>(e, component);
    }

    // Get component (mutable)
    template<typename T>
    T* getComponent(Entity e) {
        return cm.template get_component<T>(e);
    }

    // Check if entity has component
    template<typename T>
    bool hasComponent(Entity e) {
        return cm.template has_component<T>(e);
    }

    // Iterate over entities with single component
    template<typename T, typename Func>
    void forEach(Func&& func) {
        cm.template for_each_component<T>(std::forward<Func>(func));
    }

    // Iterate over entities with multiple components
    template<typename... Ts, typename Func>
    void forEachEntities(Func&& func) {
        cm.template for_each_components<Ts...>(std::forward<Func>(func));
    }

    EntityManager em;
    ComponentManager<Components...> cm;
};
