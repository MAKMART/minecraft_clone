#pragma once
#include "component_storage.hpp"
#include "entity.hpp"
#include <vector>
#include <memory>
#include <tuple>
#include <cstddef>

class ComponentManager {
public:
    template <typename T>
    void add_component(Entity e, T comp) {
        get_storage<T>()->add(e, std::move(comp));
    }

    template <typename T>
    T* get_component(Entity e) {
        return get_storage<T>()->get(e);
    }

    template <typename T>
    bool has_component(Entity e) {
        return get_storage<T>()->has(e);
    }

    template <typename T>
    void remove_component(Entity e) {
        get_storage<T>()->remove_entity(e);
    }

    // Single-component iteration – uses packed dense arrays for optimal cache performance
    template <typename T, typename Func>
    void for_each_component(Func&& func) {
        get_storage<T>()->iterate(std::forward<Func>(func));
    }

    // Multi-component iteration – uses first component type as base (put the most restrictive/rarest first for best perf)
    template <typename... Components, typename Func>
    void for_each_components(Func&& func) {
        static_assert(sizeof...(Components) > 0, "At least one component required");

        // Create tuple of all storages (lazy-creates any missing ones)
        auto storages_tuple = std::make_tuple(get_storage<Components>()...);

        // Use the first component's storage as iteration base
        using First = std::tuple_element_t<0, std::tuple<Components...>>;
        auto* base_storage = std::get<ComponentStorage<First>*>(storages_tuple);

        for (Entity e : base_storage->all_entities()) {
            // Fold-expression check: all required components present?
            if ((std::get<ComponentStorage<Components>*>(storages_tuple)->has(e) && ...)) {
                // Fold-expression unpack: call func with all components
                func(e, *std::get<ComponentStorage<Components>*>(storages_tuple)->get(e)...);
            }
        }
    }

    void remove_entity(Entity e) {
        for (auto& storage : typed_storages) {
            if (storage) storage->remove_entity(e);
        }
    }

    void clear() {
        for (auto& storage : typed_storages) {
            if (storage) storage->clear();
        }
    }

private:
    template <typename T>
    ComponentStorage<T>* get_storage() {
        const std::size_t type_id = ComponentTypeID<T>();  // <-- changed 'constexpr' to 'const'
        if (type_id >= typed_storages.size()) {
            typed_storages.resize(type_id + 1);
        }
        if (!typed_storages[type_id]) {
            typed_storages[type_id] = std::make_unique<ComponentStorage<T>>();
        }
        return static_cast<ComponentStorage<T>*>(typed_storages[type_id].get());
    }

    template <typename T>
    static std::size_t ComponentTypeID() {
        static std::size_t id = counter++;
        return id;
    }

    static inline std::size_t counter = 0;

    std::vector<std::unique_ptr<IComponentStorage>> typed_storages;
};
