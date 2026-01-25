#pragma once
#include "entity.hpp"
#include <vector>

class EntityManager
{
	public:
		EntityManager(std::size_t max_entities) { 
			free_ids.reserve(max_entities);
		}
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

		void destroy(Entity e) noexcept
		{
			free_ids.push_back(e.id);
		}
		void clear() noexcept
		{
			next_id = 0;
			free_ids.clear();
		}

	private:
		std::uint32_t             next_id = 0;
		std::vector<std::uint32_t> free_ids;
};
