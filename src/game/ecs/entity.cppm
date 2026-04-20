export module ecs:entity;

import std;
export using entity_id = std::uint32_t;
export entity_id invalid_entity_id = std::numeric_limits<entity_id>::max();
export struct Entity {
	entity_id id = invalid_entity_id;
	bool operator<=>(const Entity& other) const noexcept = default;
  bool is_valid() const noexcept { return id != invalid_entity_id; }
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
