export module engine.ecs:entity;

import std;
import engine.core;
export namespace engine::ecs {
  using entity_id = i32;
  // Plenty of range, could be u16 if you care a lot about memory and have lots of entities
  using generation_t = u32;
  entity_id invalid_entity_id = std::numeric_limits<entity_id>::max();
  struct Entity final {
    entity_id id{invalid_entity_id};
    generation_t generation{};
    bool operator<=>(const Entity& other) const noexcept = default;
  };
}
using namespace engine::ecs;
// Provide a hash function for std::unordered_map
export namespace std {
  template <>
    struct hash<Entity> {
      std::size_t operator()(const Entity& e) const noexcept {
        return std::hash<u64>{}((static_cast<u64>(e.id) << 32) | e.generation);
      }
    };
} // namespace std
