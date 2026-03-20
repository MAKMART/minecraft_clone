module;
/*
#include <vector>
#include <cstdint>
#include <algorithm>
#include <memory>
#include <tuple>
#include <utility>
*/
#include <cstddef>
#include <cstdint>
export module ecs;

// 'export import' makes 'Entity' visible to main.cpp
export import :entity;
#if defined(DEBUG)
import logger;
#endif
import core;
import std;


export struct IComponentStorage {
    virtual ~IComponentStorage() = default;
    virtual void remove_entity(Entity e) = 0;
    virtual void clear() = 0;
};

export template <typename T>
class ComponentStorage : public IComponentStorage {
public:
    void add(Entity e, T comp) {
        if (e.id >= sparse.size()) sparse.resize(e.id + 1, -1);
        entity_id& slot = sparse[e.id];
        if (slot == -1) {
            slot = static_cast<int64_t>(dense.size());
            dense.push_back(e);
            data.push_back(std::move(comp));
        } else {
            data[slot] = std::move(comp);
        }
    }

    bool has(Entity e) const {
        return e.id < sparse.size() && sparse[e.id] != -1;
    }

    T* get(Entity e) {
		if (!has(e)) return nullptr;
		return &data[sparse[e.id]];
    }

    const T* get(Entity e) const {
        if (!has(e)) return nullptr;
        return &data[sparse[e.id]];
    }

    void remove_entity(Entity e) override {
        if (!has(e)) return;
        size_t idx = static_cast<size_t>(sparse[e.id]);
        size_t last = dense.size() - 1;

        dense[idx] = dense[last];
        data[idx] = std::move(data[last]);

        sparse[dense[idx].id] = static_cast<entity_id>(idx);

        dense.pop_back();
        data.pop_back();
        sparse[e.id] = -1;
    }

    const std::vector<Entity>& all_entities() const { return dense; }

    void clear() override {
        dense.clear();
        data.clear();
        sparse.clear();  // Release memory instead of just filling with -1
    }

    // Direct packed iteration – best cache locality, no sparse lookups
    template <typename Func>
    void iterate(Func&& func) {
        for (size_t i = 0; i < dense.size(); ++i) {
            func(dense[i], data[i]);
        }
    }

    template <typename Func>
    void iterate(Func&& func) const {
        for (size_t i = 0; i < dense.size(); ++i) {
            func(dense[i], data[i]);
        }
    }

private:
    std::vector<entity_id> sparse; // entity.id → dense index (-1 if absent)
    std::vector<Entity> dense;   // packed entities
    std::vector<T> data;         // packed components
};
export class ComponentManager {
public:
    template <typename T>
    void add_component(Entity e, T comp) noexcept {
        get_storage<T>()->add(e, std::move(comp));
    }

    template <typename T>
    T* get_component(Entity e) noexcept {
        return get_storage<T>()->get(e);
    }

    template <typename T>
    const T* get_component(Entity e) const noexcept {
        return get_storage<T>()->get(e);
    }

    template <typename T>
    bool has_component(Entity e) noexcept {
        return get_storage<T>()->has(e);
    }

    template <typename T>
    void remove_component(Entity e) noexcept {
        get_storage<T>()->remove_entity(e);
    }

    // Single-component iteration – uses packed dense arrays for optimal cache performance
    template <typename T, typename Func>
    void for_each_component(Func&& func) noexcept {
        get_storage<T>()->iterate(std::forward<Func>(func));
    }

    // Multi-component iteration – uses first component type as base (put the most restrictive/rarest first for best perf)
    template <typename... Components, typename Func>
    void for_each_components(Func&& func) {
        static_assert(sizeof...(Components) > 0, "At least one component required");

        // Create tuple of all storages (lazy-creates any missing ones)
        auto storages_tuple = std::make_tuple(this->template get_storage<Components>()...);

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

    void remove_entity(Entity e) noexcept {
		for (auto& storage : typed_storages)
			if (storage) storage->remove_entity(e);
    }

    void clear() noexcept {
		for (auto& storage : typed_storages)
			if (storage) storage->clear();
    }

private:
    template <typename T>
    ComponentStorage<T>* get_storage() noexcept {
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
    const ComponentStorage<T>* get_storage() const noexcept {
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
		// Maybe if you change to ++counter you will start counting components from index 1 ??
        static std::size_t id = counter++;
        return id;
    }

    static inline std::size_t counter = 0;

    mutable std::vector<std::unique_ptr<IComponentStorage>> typed_storages;
};

export class EntityManager
{
	public:
		constexpr EntityManager(std::size_t max_entities) { free_ids.reserve(max_entities); }

		Entity create() noexcept
		{
			Entity e;
			if (!free_ids.empty()) {
				e.id = free_ids.back();
				free_ids.pop_back();
			} else {
				e.id = next_id++;
			}
			return e;
		}

		void destroy(Entity e) noexcept { free_ids.push_back(e.id); }
		void clear() noexcept {
			next_id = 0;
			free_ids.clear();
		}

	private:
		entity_id next_id = 0;
		std::vector<entity_id> free_ids;
};

export class ECS
{
      public:
		  ECS() noexcept = default;
		  ECS(const ECS&) = delete;
		  ECS& operator=(const ECS&) = delete;
		  ECS(ECS&&) noexcept = default;
		  ECS& operator=(ECS&&) noexcept = default;
	[[nodiscard]] inline Entity create_entity() noexcept { return em.create(); }
	inline void destroy_entity(Entity e)
	{
		em.destroy(e);
		cm.remove_entity(e);
	}

	template <typename T>
	inline void add_component(Entity e, const T& c)
	{
		cm.add_component<T>(e, c);
	}

	template <typename T, typename... Args>
	inline void emplace_component(Entity e, Args&&... args)
	{
		cm.add_component<T>(e, T(std::forward<Args>(args)...));
	}

	template <typename T>
	inline void remove_component(Entity e)
	{
		cm.remove_component<T>(e);
	}

	template <typename T>
	[[nodiscard]] inline T* get_component(Entity e)
	{
		T* ptr = cm.template get_component<T>(e);
#if defined(DEBUG)
		if (!ptr) {
			log::system_error("ECS", "Failed to get component {}, for entity with id: {}", type_name<T>(), e.id);
		}
#endif
		return ptr;
	}

	template <typename T>
	[[nodiscard]] inline const T* get_component(Entity e) const
	{
		const T* ptr = cm.get_component<T>(e);
#if defined(DEBUG)
		if (!ptr) {
			log::system_error("ECS", "Failed to get component {}, for entity with id: {}", type_name<T>(), e.id);
		}
#endif
		return ptr;
	}

	template <typename T, typename Func>
	inline void for_each_component(Func&& func)
	{
		cm.for_each_component<T>(std::forward<Func>(func));
	}

	template <typename... Components, typename Func>
	inline void for_each_components(Func&& func)
	{
		cm.for_each_components<Components...>(std::forward<Func>(func));
	}

	template <typename T>
	[[nodiscard]] inline bool has_component(Entity e)
	{
		return cm.has_component<T>(e);
	}

	template <typename... Ts>
	[[nodiscard]] inline bool has_components(Entity e)
	{
		return (has_component<Ts>(e) && ...);
	}

	template <typename... Ts, typename Func>
		void for_entities_with(Func&& func)
		{
			for_each_components<Ts...>([&](Entity e, Ts&...) {
					func(e);
					});
		}
	template <typename... Ts>
	std::vector<Entity> get_entities_with()
	{
		std::vector<Entity> result;

		if constexpr (sizeof...(Ts) > 0) {
			size_t min_size = std::min({ cm.get_storage<Ts>()->all_entities().size() ... });
			result.reserve(min_size);
		}

		for_each_components<Ts...>([&](Ts&..., Entity e) {
			result.push_back(e);
		});

		return result;
	}

	template <typename T>
	size_t component_count() const
	{
		return cm.get_storage<T>()->all_entities().size();
	}
	void clear()
	{
		em.clear();
		cm.clear();
	}

      private:
	EntityManager    em{MAX_ENTITIES};
	ComponentManager cm;
};
