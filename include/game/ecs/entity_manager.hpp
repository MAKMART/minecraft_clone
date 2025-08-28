#pragma once
#include "entity.hpp"
#include <queue>

class EntityManager
{
      public:
	Entity create()
	{
		Entity e;
		if (!free_ids.empty()) {
			e.id = free_ids.front();
			free_ids.pop();
		} else {
			e.id = next_id++;
		}
		return e;
	}

	void destroy(Entity e)
	{
		free_ids.push(e.id);
	}

      private:
	std::uint32_t             next_id = 0;
	std::queue<std::uint32_t> free_ids;
};
