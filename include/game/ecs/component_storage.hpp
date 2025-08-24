#pragma once
#include "entity.hpp"
#include <vector>
#include <unordered_map>

template<typename T>
class ComponentStorage {
public:
    void add(Entity e, const T& component) {
        if (entity_to_index.find(e.id) != entity_to_index.end()) {
            data[entity_to_index[e.id]] = component;
            return;
        }
        data.push_back(component);
        entities.push_back(e);
        entity_to_index[e.id] = data.size() - 1;
    }

    bool has(Entity e) const {
        return entity_to_index.count(e.id) > 0;
    }

    T* get(Entity e) {
        auto it = entity_to_index.find(e.id);
        if (it == entity_to_index.end()) return nullptr;
        return &data[it->second];
    }

    const T* get(Entity e) const {
	    auto it = entity_to_index.find(e.id);
	    if (it == entity_to_index.end()) return nullptr;
	    return &data[it->second];
    }

    void remove(Entity e) {
        auto it = entity_to_index.find(e.id);
        if (it == entity_to_index.end()) return;

        size_t index = it->second;
        size_t last = data.size() - 1;

        if (index != last) {
            // swap last element into the removed spot
            data[index] = data[last];
            entities[index] = entities[last];
            entity_to_index[entities[index].id] = index;
        }

        data.pop_back();
        entities.pop_back();
        entity_to_index.erase(it);
    }

    // iterate safely over active components
    std::vector<T>& all_components() { return data; }
    std::vector<Entity>& all_entities() { return entities; }

private:
    std::vector<T> data;
    std::vector<Entity> entities;
    std::unordered_map<entity_id, size_t> entity_to_index;
};

