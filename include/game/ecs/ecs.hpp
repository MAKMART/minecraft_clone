#pragma once
#include "entity_manager.hpp"
#include "component_manager.hpp"
#include "core/defines.hpp"
#include <cassert>
#include <algorithm>
#if defined(DEBUG)
#include "core/logger.hpp"
#endif

class ECS
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
		T* ptr = cm.get_component<T>(e);
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
