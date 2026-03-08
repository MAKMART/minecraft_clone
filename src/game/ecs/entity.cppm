module;
#include <cstdint>
#include <cstddef>
#include <functional>
export module ecs:entity;

export using entity_id = std::uint32_t;
export struct Entity {
	entity_id id;
	bool operator==(const Entity& other) const { return id == other.id; }
};

// Provide a hash function for std::unordered_map
export namespace std
{
template <>
struct hash<Entity> {
	std::size_t operator()(const Entity& e) const noexcept
	{
		return std::hash<entity_id>{}(e.id);
	}
};
} // namespace std
