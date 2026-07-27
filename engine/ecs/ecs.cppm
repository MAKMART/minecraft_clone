module;
#include <cassert>

#include <algorithm>      // std::fill, std::min
#include <concepts>       // std::invocable
#include <cstddef>        // std::size_t
#include <limits>         // std::numeric_limits
#include <memory>         // std::unique_ptr, std::make_unique
#include <span>           // std::span
#include <string_view>    // std::string_view
#include <tuple>          // std::tuple, std::make_tuple, std::get, std::tuple_element_t
#include <type_traits>    // std::remove_cvref_t
#include <utility>        // std::move, std::forward
#include <vector>         // std::vector
export module engine.ecs;

// 'export import' makes 'Entity' visible to importers
export import :entity;
import engine.core;

using namespace engine::core;

export namespace engine::ecs {
  class component {
    protected:
      constexpr component() = default;
  };

  template <typename T>
    concept component_type = std::derived_from<T, component>;
}



  template <typename T>
    constexpr std::string_view type_name() {
      // TODO: Use reflection when available
#if defined(__clang__) || defined(__GNUC__)
      std::string_view p     = __PRETTY_FUNCTION__;
      auto             start = p.find("T = ") + 4;
      auto             end   = p.find(';', start);
      return p.substr(start, end - start);
#elif defined(_MSC_VER)
      std::string_view p     = __FUNCSIG__;
      auto             start = p.find("type_name<") + 10;
      auto             end   = p.find(">(void)", start);
      return p.substr(start, end - start);
#endif
      // std::meta::identifier_of(^^T);
    }

struct IComponentStorage {
  virtual ~IComponentStorage() = default;
  virtual void remove_entity(Entity e) = 0;
  virtual void clear() = 0;
};

template <component_type Component>
class ComponentStorage : public IComponentStorage {
  public:
    explicit ComponentStorage(std::size_t max_entities) {
      sparse.resize(max_entities, invalid_entity_id);
      dense.reserve(max_entities);
      data.reserve(max_entities);
    }
    void add(Entity e, Component comp) {
      assert(e.id < sparse.size());
      entity_id& slot = sparse[e.id];
      if (slot == invalid_entity_id) {
        slot = static_cast<i64>(dense.size());
        dense.emplace_back(e);
        data.push_back(std::move(comp));
      } else {
        data[slot] = std::move(comp);
      }
    }
    template <typename... Args>
      Component& emplace(Entity e, Args&&... args)
      {
        assert(e.id < sparse.size());
        entity_id& slot{sparse[e.id]};

        if (slot == invalid_entity_id) {
          slot = static_cast<entity_id>(dense.size());
          dense.emplace_back(e);
          data.emplace_back(std::forward<Args>(args)...);
          return data.back();
        }
        else {
          data[slot] = Component(std::forward<Args>(args)...);
          return data[slot];
        }
      }

    inline bool has(Entity e) const noexcept {
      return e.id < sparse.size() &&
        sparse[e.id] != invalid_entity_id &&
        dense[sparse[e.id]].generation == e.generation;
    }

    Component* get(Entity e) {
      if (!has(e)) return nullptr;
      return &data[sparse[e.id]];
    }

    const Component* get(Entity e) const {
      if (!has(e)) return nullptr;
      return &data[sparse[e.id]];
    }

    void remove_entity(Entity e) override {
      if (!has(e)) return;
      std::size_t idx{static_cast<std::size_t>(sparse[e.id])};
      std::size_t last{dense.size() - 1};

      if (idx != last) {
        dense[idx] = dense[last];
        data[idx] = std::move(data[last]);
        sparse[dense[idx].id] = static_cast<entity_id>(idx);
      }

      dense.pop_back();
      data.pop_back();
      sparse[e.id] = invalid_entity_id;
    }

    const std::span<const Entity> all_entities() const { return dense; }

    void clear() override {
      dense.clear();
      data.clear();
      std::fill(sparse.begin(), sparse.end(), invalid_entity_id);
    }

    // Direct packed iteration – best cache locality, no sparse lookups
    template<typename F> requires std::invocable<F, Entity, Component&>
      void iterate(F&& func) {
        for (std::size_t i{0}; i < dense.size(); ++i) {
          func(dense[i], data[i]);
        }
      }

    template<typename F> requires std::invocable<F, Entity, Component&>
      void iterate(F&& func) const {
        for (std::size_t i{0}; i < dense.size(); ++i) {
          func(dense[i], data[i]);
        }
      }

  private:
    std::vector<entity_id> sparse; // entity.id → dense index (invalid_entity_id if absent)
    std::vector<Entity> dense;   // packed entities
    std::vector<Component> data;         // packed components
};
class ComponentManager {
  public:
    constexpr explicit ComponentManager(std::size_t max_entities) : max_entities(max_entities) {
      typed_storages.reserve(max_entities);
    }
      // inline void add_component(Entity e, auto comp) noexcept {
      //   get_storage<decltype(comp)>()->add(e, std::move(comp));
      // }

      inline void add_component(Entity e, auto&& comp) noexcept {
        using T = std::remove_cvref_t<decltype(comp)>;

        get_storage<T>()->add(e, std::forward<decltype(comp)>(comp));
      }
    template <component_type Component, typename... Args>
      void emplace_component(Entity e, Args&&... args)
      {
        get_storage<Component>()->emplace(e, std::forward<Args>(args)...);
      }

    template <component_type Component>
      Component* get_component(Entity e) noexcept {
        // get will return nullptr
        return get_storage<Component>()->get(e);
      }

    template <component_type Component>
      bool has_component(Entity e) noexcept {
        return get_storage<Component>()->has(e);
      }

    template <component_type Component>
      void remove_component(Entity e) noexcept {
        get_storage<Component>()->remove_entity(e);
      }

    // Single-component iteration – uses packed dense arrays for optimal cache performance
    template <component_type Component>
      void for_each_component(auto&& func) noexcept {
        get_storage<Component>()->iterate(std::forward<decltype(func)>(func));
      }

    // Multi-component iteration – uses first component type as base (put the most restrictive/rarest first for best perf)
    template <component_type... Components> requires (sizeof...(Components) > 0)
      void for_each_components(auto&& func) {
        // Create tuple of all storages (lazy-creates any missing ones)
        auto storages_tuple{std::make_tuple(this->template get_storage<Components>()...)};

        // Use the first component's storage as iteration base
        // using First = std::tuple_element_t<0, std::tuple<Components...>>;
        using First = Components...[0];
        auto* base_storage{std::get<ComponentStorage<First>*>(storages_tuple)};

        for (Entity e : base_storage->all_entities()) {
          // Fold-expression check: all required components present?
          if ((std::get<ComponentStorage<Components>*>(storages_tuple)->has(e) && ...)) {
            // Fold-expression unpack: call func with all components
            func(e, *std::get<ComponentStorage<Components>*>(storages_tuple)->get(e)...);
          }
        }
      }

    void remove_entity(Entity e) noexcept {
      for (auto& storage : typed_storages)
        if (storage) storage->remove_entity(e);
    }

    void clear() noexcept {
      for (auto& storage : typed_storages)
        if (storage) storage->clear();
    }

  private:
    template <component_type Component>
      ComponentStorage<Component>* get_storage() {
        const std::size_t type_id{ComponentTypeID<Component>()};
        if (type_id >= typed_storages.size()) {
          typed_storages.resize(type_id + 1);
        }
        if (!typed_storages[type_id]) {
          typed_storages[type_id] = std::make_unique<ComponentStorage<Component>>(max_entities);
        }
        return static_cast<ComponentStorage<Component>*>(typed_storages[type_id].get());
      }


    template <component_type Component>
      const ComponentStorage<Component>* get_storage() const noexcept {
        const std::size_t type_id{ComponentTypeID<Component>()};

        if (type_id >= typed_storages.size())
          return nullptr;

        return static_cast<const ComponentStorage<Component>*>(typed_storages[type_id].get());
      }

    template <component_type Component>
      static std::size_t ComponentTypeID() {
        static std::size_t id{counter++};
        return id;
      }

    std::size_t max_entities{};
    static inline std::size_t counter{0};
    std::vector<std::unique_ptr<IComponentStorage>> typed_storages;
};

class EntityManager {
  public:
    constexpr explicit EntityManager(std::size_t max_entities) : max_entities(max_entities) { free_ids.reserve(max_entities); }
    [[nodiscard("Use the entity you just created!")]] Entity create() noexcept {
      if (static_cast<std::size_t>(next_id) >= max_entities && free_ids.empty())
        return {};
      entity_id id;
      if (!free_ids.empty()) {
        id = free_ids.back();
        free_ids.pop_back();
      } else {
        id = next_id++;
        generations.push_back(0);
      }
      return { id, generations[id] };
    }
    void destroy(Entity e) noexcept {
      if (e.id >= next_id) return;
      if (!is_alive(e)) return;
      ++generations[e.id];
      free_ids.push_back(e.id);
    }
    inline bool is_alive(Entity e) const noexcept {
      return static_cast<std::size_t>(e.id) < generations.size() && generations[e.id] == e.generation;
    }
    void clear() noexcept {
      next_id = 0;
      free_ids.clear();
      generations.clear();
    }

  private:
    entity_id next_id{0};
    std::size_t max_entities;
    std::vector<entity_id> free_ids;
    std::vector<generation_t> generations;
};








export namespace engine::ecs {
    class ECS {
      public:
        explicit ECS(std::size_t max_entities) noexcept
        : max_entities(max_entities), em(max_entities), cm(max_entities) {}
        ECS(const ECS&) = delete;
        ECS& operator=(const ECS&) = delete;
        ECS(ECS&&) noexcept = default;
        ECS& operator=(ECS&&) noexcept = default;
        [[nodiscard]] inline Entity create_entity() noexcept { return em.create(); }
        inline void destroy_entity(Entity e) {
          em.destroy(e);
          cm.remove_entity(e);
        }

        inline bool is_alive(Entity e) const noexcept {
          return em.is_alive(e);
        }



        template <typename Component> requires component_type<std::remove_cvref_t<Component>>
          inline bool add_component(Entity e, Component&& c) {
            if (!em.is_alive(e))
              return false;
            cm.add_component(e, std::forward<Component>(c));
            return true;
          }
        template <component_type Component, typename... Args>
          inline bool add_component_if_missing(Entity e, Args&&... args) noexcept {
            if (!em.is_alive(e))
              return false;
            if (!has_component<Component>(e)) {
              emplace_component<Component>(e, std::forward<Args>(args)...);
              return true;
            }
            return false;
          }

        template <component_type Component, typename... Args>
          inline bool emplace_component(Entity e, Args&&... args) {
            if (!em.is_alive(e))
              return false;
            cm.emplace_component<Component>(e, std::forward<Args>(args)...);
            return true;
          }

        template <component_type Component>
          inline bool remove_component(Entity e) {
            if (!em.is_alive(e))
              return false;
            cm.remove_component<Component>(e);
            return true;
          }

        template <component_type Component>
          [[nodiscard]] inline Component* get_component(Entity e) {
            Component* ptr{cm.get_component<Component>(e)};
#if defined(DEBUG)
            if (!ptr) {
              // auto name = type_name<Component>();
              // logger::system_error("ECS", "Failed to get component {}, for entity with id: {}", name, e.id);
            }
#endif
            return ptr;
          }

        template <component_type Component>
          [[nodiscard]] inline const Component* get_component(Entity e) const {
            const Component* ptr{cm.get_component<Component>(e)};
#if defined(DEBUG)
            if (!ptr) {
              // auto name = type_name<Component>();
              // logger::system_error("ECS", "Failed to get component {}, for entity with id: {}", name, e.id);
            }
#endif
            return ptr;
          }

        template <component_type Component>
          inline void for_each_component(auto&& func) {
            cm.for_each_component<Component>(std::forward<decltype(func)>(func));
          }

        template <component_type... Components> requires (sizeof...(Components) > 0)
          inline void for_each_components(auto&& func) {
            cm.for_each_components<Components...>(std::forward<decltype(func)>(func));
          }

        template <component_type Component>
          [[nodiscard]] inline bool has_component(Entity e) {
            if (!em.is_alive(e))
              return false;
            return cm.has_component<Component>(e);
          }

        template <component_type... Components> requires (sizeof...(Components) > 0)
          [[nodiscard]] inline bool has_components(Entity e) {
            if (!em.is_alive(e))
              return false;
            return (has_component<Components>(e) && ...);
          }

        template <component_type... Components> requires (sizeof...(Components) > 0)
          void for_entities_with(auto&& func) {
            for_each_components<Components...>([&](Entity e, Components&...) {
                func(e);
                });
          }
        template <component_type... Components> requires (sizeof...(Components) > 0)
          std::vector<Entity> get_entities_with() {
            std::vector<Entity> result;

            std::size_t min_size{std::min({ cm.get_storage<Components>()->all_entities().size() ... })};
            result.reserve(min_size);

            for_each_components<Components...>([&](Entity e, Components&...) {
                result.push_back(e);
                });

            return result;
          }

        template <component_type Component>
          std::size_t component_count() {
            return cm.get_storage<Component>()->all_entities().size();
          }

        void clear() {
          em.clear();
          cm.clear();
        }

      private:
        std::size_t max_entities{std::numeric_limits<std::size_t>::max()};
        EntityManager    em;
        ComponentManager cm;
    };
}
