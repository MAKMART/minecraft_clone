module;
#include <cstddef>
#include <cassert>
export module ecs;

// 'export import' makes 'Entity' visible to importers
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
  constexpr explicit ComponentStorage(std::size_t max_entities) { 
    sparse.resize(max_entities, static_cast<entity_id>(-1)); 
    dense.reserve(max_entities);
    data.reserve(max_entities);
  }
    void add(Entity e, T comp) {
      assert(e.id < sparse.size());
        entity_id& slot = sparse[e.id];
        if (slot == -1) {
            slot = static_cast<std::int64_t>(dense.size());
            dense.emplace_back(e);
            data.push_back(std::move(comp));
        } else {
            data[slot] = std::move(comp);
        }
    }
    template <typename... Args>
      T& emplace(Entity e, Args&&... args)
      {
        assert(e.id < sparse.size());
        entity_id& slot = sparse[e.id];

        if (slot == static_cast<entity_id>(-1))
        {
          slot = static_cast<entity_id>(dense.size());
          dense.emplace_back(e);
          data.emplace_back(std::forward<Args>(args)...);
          return data.back();
        }
        else
        {
          data[slot] = T(std::forward<Args>(args)...);
          return data[slot];
        }
      }

    inline bool has(Entity e) const noexcept {
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

        if (idx != last)
        {
          dense[idx] = dense[last];
          data[idx] = std::move(data[last]);
          sparse[dense[idx].id] = static_cast<entity_id>(idx);
        }

        dense.pop_back();
        data.pop_back();
        sparse[e.id] = static_cast<entity_id>(-1);
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
  constexpr explicit ComponentManager(std::size_t max_entities)
    : max_entities(max_entities) {}
    template <typename T>
    inline void add_component(Entity e, T comp) noexcept {
        get_storage<T>()->add(e, std::move(comp));
    }
    template <typename T, typename... Args>
      void emplace_component(Entity e, Args&&... args) noexcept
      {
        get_storage<T>()->emplace(e, std::forward<Args>(args)...);
      }

    template <typename T>
    T* get_component(Entity e) noexcept {
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
            typed_storages[type_id] = std::make_unique<ComponentStorage<T>>(max_entities);
        }
        return static_cast<ComponentStorage<T>*>(typed_storages[type_id].get());
    }
    /* This must not allocate
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
    */

    template <typename T>
    static std::size_t ComponentTypeID() {
        static std::size_t id = counter++;
        return id;
    }

    static inline std::size_t counter = 0;
    std::vector<std::unique_ptr<IComponentStorage>> typed_storages;
    std::size_t max_entities = std::numeric_limits<std::size_t>::max();
};

export class EntityManager
{
	public:
		constexpr explicit EntityManager(std::size_t max_entities) { free_ids.reserve(max_entities); }

    Entity create() noexcept
    {
      Entity e;
      if (!free_ids.empty())
      {
        e.id = free_ids.back();
        free_ids.pop_back();
      }
      else
      {
        e.id = next_id++;
      }

      return e;
    }
    void destroy(Entity e) noexcept
    {
      if (e.id >= next_id) return;
      if (!e.is_valid()) return;
      free_ids.push_back(e.id);
    }
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
  inline void add_component_if_missing(Entity e, Args&&... args) noexcept
  {
    if (!has_component<T>(e)) {
      emplace_component<T>(e, std::forward<Args>(args)...);
    }
  }

	template <typename T, typename... Args>
	inline void emplace_component(Entity e, Args&&... args)
	{
		// cm.add_component<T>(e, T(std::forward<Args>(args)...));
    cm.emplace_component<T>(e, std::forward<Args>(args)...);
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
	ComponentManager cm{MAX_ENTITIES};
};
