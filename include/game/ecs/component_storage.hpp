#pragma once
#include "entity.hpp"
#include <vector>
#include <cstddef>
#include <utility>
#include <tuple>

struct IComponentStorage {
    virtual ~IComponentStorage() = default;
    virtual void remove_entity(Entity e) = 0;
    virtual void clear() = 0;
};

template <typename T>
class ComponentStorage : public IComponentStorage {
public:
    void add(Entity e, T comp) {
        if (e.id >= sparse.size()) sparse.resize(e.id + 1, -1);
        int64_t& slot = sparse[e.id];
        if (slot == -1) {
            slot = static_cast<int64_t>(dense.size());
            dense.push_back(e);
            data.push_back(std::move(comp));
        } else {
            data[slot] = std::move(comp);
        }
    }

    bool has(Entity e) const {
        return e.id < sparse.size() && sparse[e.id] != -1;
    }

    T* get(Entity e) {
        return const_cast<T*>(std::as_const(*this).get(e));
    }

    const T* get(Entity e) const {
        if (!has(e)) return nullptr;
        return &data[sparse[e.id]];
    }

    void remove_entity(Entity e) override {
        if (!has(e)) return;
        size_t idx = static_cast<size_t>(sparse[e.id]);
        size_t last = dense.size() - 1;

        dense[idx] = dense[last];
        data[idx] = std::move(data[last]);

        sparse[dense[idx].id] = static_cast<int64_t>(idx);

        dense.pop_back();
        data.pop_back();
        sparse[e.id] = -1;
    }

    const std::vector<Entity>& all_entities() const { return dense; }

    void clear() override {
        dense.clear();
        data.clear();
        sparse.clear();  // Release memory instead of just filling with -1
    }

    // Direct packed iteration – best cache locality, no sparse lookups
    template <typename Func>
    void iterate(Func&& func) {
        for (size_t i = 0; i < dense.size(); ++i) {
            func(dense[i], data[i]);
        }
    }

    template <typename Func>
    void iterate(Func&& func) const {
        for (size_t i = 0; i < dense.size(); ++i) {
            func(dense[i], data[i]);
        }
    }

private:
    std::vector<int64_t> sparse; // entity.id → dense index (-1 if absent)
    std::vector<Entity> dense;   // packed entities
    std::vector<T> data;         // packed components
};
