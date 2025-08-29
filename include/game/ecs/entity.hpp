#pragma once
#include <cstdint>
using entity_id = std::uint32_t;
struct Entity {
	entity_id id;
	bool      operator==(const Entity& other) const
	{
		return id == other.id;
	}
};
// Provide a hash function for std::unordered_map
namespace std
{
template <>
struct hash<Entity> {
	std::size_t operator()(const Entity& e) const noexcept
	{
		return std::hash<entity_id>{}(e.id);
	}
};
} // namespace std
