#pragma once
#include "entity.hpp"
#include <vector>
#include <unordered_map>

struct IComponentStorage {
	virtual ~IComponentStorage()         = default;
	virtual void remove_entity(Entity e) = 0;
	virtual void clear() = 0;
};
template <typename T>
class ComponentStorage : public IComponentStorage
{
      public:
	void add(Entity e, const T& component)
	{
		data.emplace(e, component);
	}

	bool has(Entity e) const
	{
		return data.find(e) != data.end();
	}

	T* get(Entity e)
	{
		auto it = data.find(e);
		if (it != data.end())
			return &it->second;
		return nullptr;
	}

	const T* get(Entity e) const
	{
		auto it = data.find(e);
		if (it != data.end())
			return &it->second;
		return nullptr;
	}

	void remove_entity(Entity e) override
	{
		data.erase(e);
	}

	std::vector<Entity> all_entities() const
	{
		std::vector<Entity> entities;
		for (auto& [e, _] : data)
			entities.push_back(e);
		return entities;
	}

	void clear()
	{
		data.clear(); // frees all T objects, keeps the map ready for reuse
	}

      private:
	std::unordered_map<Entity, T> data;
};
