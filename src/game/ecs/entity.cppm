export module ecs:entity;

import std;
export using entity_id = std::uint32_t;
export struct Entity {
	entity_id id = std::numeric_limits<entity_id>::max();
	bool operator<=>(const Entity& other) const noexcept = default;
  bool is_valid() const noexcept { return id != std::numeric_limits<entity_id>::max(); }
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
