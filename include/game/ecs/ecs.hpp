#pragma once
#include "entity_manager.hpp"
#include "component_manager.hpp"
#include "core/defines.hpp"
#if defined(DEBUG)
#include "core/logger.hpp"
#endif

class ECS
{
      public:
	Entity create_entity()
	{
		return em.create();
	}
	void destroy_entity(Entity e)
	{
		em.destroy(e);
		cm.remove_entity(e);
	}

	template <typename T>
	void add_component(Entity e, const T& c)
	{
		cm.add_component<T>(e, c);
	}

	template <typename T>
	void remove_component(Entity e)
	{
		cm.remove_component<T>(e);
	}

	template <typename T>
	T* get_component(Entity e)
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
	const T* get_component(Entity e) const
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
	void for_each_component(Func&& func)
	{
		cm.for_each_component<T>(std::forward<Func>(func));
	}

	template <typename... Components, typename Func>
	void for_each_components(Func&& func)
	{
		cm.for_each_components<Components...>(std::forward<Func>(func));
	}

	template <typename T>
	bool has_component(Entity e)
	{
		return cm.has_component<T>(e);
	}

	template <typename... Ts>
	bool has_components(Entity e)
	{
		return (has_component<Ts>(e) && ...);
	}

	template <typename... Ts>
	std::vector<Entity> get_entities_with()
	{
		std::vector<Entity> result;

		for_each_components<Ts...>([&](Ts&..., Entity e) {
			result.push_back(e);
		});

		return result;
	}

	template <typename T>
	size_t component_count()
	{
		return cm.get_storage<T>()->all_entities().size();
	}
	void clear()
	{
		em.clear();
		cm.clear();
	}

      private:
	EntityManager    em;
	ComponentManager cm;
};
