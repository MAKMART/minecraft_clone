#pragma once
#include "component_storage.hpp"
#include "entity.hpp"
#include <memory>
#include <unordered_map>
#include <tuple>
#include <typeindex>

class ComponentManager
{
      public:
	template <typename T>
	void add_component(Entity e, const T& component)
	{
		get_storage<T>()->add(e, component);
	}

	template <typename T>
	T* get_component(Entity e)
	{
		return get_storage<T>()->get(e);
	}

	template <typename T>
	bool has_component(Entity e)
	{
		return get_storage<T>()->has(e);
	}

	template <typename T>
	void remove_component(Entity e)
	{
		get_storage<T>()->remove(e);
	}

	template <typename T, typename Func>
	void for_each_component(Func&& func)
	{
		auto* storage = get_storage<T>();
		for (Entity e : storage->all_entities()) {
			T* comp = storage->get(e);
			if (comp)
				func(e, *comp);
		}
	}

	// Iterate over multiple components (basic runtime filtering)
	template <typename... Components, typename Func>
	void for_each_components(Func&& func)
	{
		if constexpr (sizeof...(Components) == 0)
			return;

		// Cache storage pointers in a tuple
		auto storages_tuple = std::make_tuple(get_storage<Components>()...);

		// Pick first storage as iteration base
		auto* firstStorage = std::get<0>(storages_tuple);

		for (Entity e : firstStorage->all_entities()) {
			// Check if all components exist
			if ((std::get<ComponentStorage<Components>*>(storages_tuple)->has(e) && ...)) {
				// Dereference and call function
				func(e, *std::get<ComponentStorage<Components>*>(storages_tuple)->get(e)...);
			}
		}
	}

	void remove_entity(Entity e)
	{
		for (auto& [_, storage] : storages) {
			storage->remove_entity(e);
		}
	}

	void clear()
	{
		storages.clear(); // destroys all ComponentStorage<T>, frees memory
	}

	void reset_components()
	{
		for (auto& [_, storage] : storages)
			storage->clear(); // just clears component data, keeps storage alive
	}

      private:
	std::unordered_map<std::type_index, std::unique_ptr<IComponentStorage>> storages;

	template <typename T>
	ComponentStorage<T>* get_storage()
	{
		auto type = std::type_index(typeid(T));
		auto it   = storages.find(type);
		if (it == storages.end()) {
			auto storage   = std::make_unique<ComponentStorage<T>>();
			auto ptr       = storage.get();
			storages[type] = std::move(storage);
			return ptr;
		}
		return static_cast<ComponentStorage<T>*>(it->second.get());
	}
};
