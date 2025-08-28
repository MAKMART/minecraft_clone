#pragma once
#include "component_storage.hpp"
#include "entity.hpp"
#include <tuple>

template <typename... Components>
class ComponentManager
{
      public:
	template <typename T>
	void add_component(Entity e, const T& component)
	{
		get_storage<T>().add(e, component);
	}

	template <typename T>
	bool has_component(Entity e)
	{
		return std::get<ComponentStorage<T>>(storages).has(e);
	}

	template <typename T>
	T* get_component(Entity e)
	{
		return get_storage<T>().get(e);
	}

	template <typename T>
	T* get_optional_component(Entity e)
	{
		return has_component<T>(e) ? get_component<T>(e) : nullptr;
	}

	template <typename T>
	void remove_component(Entity e)
	{
		get_storage<T>().remove(e);
	}

	void remove_entity(Entity e)
	{
		(std::get<ComponentStorage<Components>>(storages).remove(e), ...); // fold expression C++17
	}

	template <typename T, typename Func>
	void for_each_component(Func&& f)
	{
		auto& storage = std::get<ComponentStorage<T>>(storages);
		for (Entity e : storage.all_entities()) {
			T* comp = storage.get(e);
			if (comp)
				f(e, *comp);
		}
	}
	template <typename... Ts, typename Func>
	void for_each_components(Func&& f)
	{
		// Get references to all storages
		auto& storagesTuple = storages;

		// We'll use the smallest storage to minimize iteration
		auto& smallest = std::get<ComponentStorage<std::tuple_element_t<0, std::tuple<Ts...>>>>(storagesTuple);

		for (Entity e : smallest.all_entities()) {
			if ((std::get<ComponentStorage<Ts>>(storagesTuple).has(e) && ...)) {
				f(e, *std::get<ComponentStorage<Ts>>(storagesTuple).get(e)...);
			}
		}
	}

	// Count of entities with a component
	template <typename T>
	size_t count() const
	{
		return std::get<ComponentStorage<T>>(storages).all_entities().size();
	}

	// Iterate only entities with one component and return vector
	template <typename T>
	std::vector<Entity> entities_with() const
	{
		return std::get<ComponentStorage<T>>(storages).all_entities();
	}

      private:
	std::tuple<ComponentStorage<Components>...> storages;

	template <typename T>
	ComponentStorage<T>& get_storage()
	{
		return std::get<ComponentStorage<T>>(storages);
	}

	template <typename T>
	const ComponentStorage<T>& get_storage() const
	{
		return std::get<ComponentStorage<T>>(storages);
	}
};
