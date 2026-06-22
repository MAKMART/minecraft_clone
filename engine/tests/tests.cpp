#include <gtest/gtest.h>

import engine.ecs;
import engine.math;
import engine.platform;

namespace ecs_tests {
  using namespace engine::ecs;
  TEST(ECS, entity_creation) {
    ECS ecs{100};
    Entity e = ecs.create_entity();
    EXPECT_TRUE(e.id != invalid_entity_id);
    EXPECT_TRUE(e.id == 0);
  }
  TEST(ECS, entity_deletion) {
    ECS ecs{100};
    Entity e = ecs.create_entity();
    ecs.destroy_entity(e);
    EXPECT_FALSE(ecs.is_alive(e));
  }
  TEST(ECS, capacity_exhaustion) {
    ECS ecs{2};

    auto e1 = ecs.create_entity();
    auto e2 = ecs.create_entity();

    auto e3 = ecs.create_entity(); // should fail or be invalid

    EXPECT_FALSE(ecs.is_alive(e3));
  }
  TEST(ECS, double_deletion_is_safe) {
    ECS ecs{100};

    auto e = ecs.create_entity();

    ecs.destroy_entity(e);
    EXPECT_FALSE(ecs.is_alive(e));

    // should not crash or corrupt
    ecs.destroy_entity(e);

    EXPECT_FALSE(ecs.is_alive(e));
  }
  TEST(ECS, entity_reuse_invalidates_old_handle) {
    ECS ecs{100};

    auto e1 = ecs.create_entity();
    ecs.destroy_entity(e1);

    auto e2 = ecs.create_entity();

    EXPECT_NE(e1, e2);                // generation differs
    EXPECT_FALSE(ecs.is_alive(e1));   // old handle dead
    EXPECT_TRUE(ecs.is_alive(e2));    // new one alive
  }
  TEST(ECS, multiple_reuse_cycles) {
    ECS ecs{10};

    auto e = ecs.create_entity();

    for (int i = 0; i < 50; ++i) {
      ecs.destroy_entity(e);
      auto next = ecs.create_entity();

      EXPECT_FALSE(ecs.is_alive(e));
      EXPECT_TRUE(ecs.is_alive(next));

      e = next;
    }
  }
  struct Position { int x, y; };

  TEST(ECS, add_get_remove_component) {
    ECS ecs{100};

    auto e = ecs.create_entity();

    EXPECT_TRUE(ecs.add_component<Position>(e, {1, 2}));

    auto* p = ecs.get_component<Position>(e);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->x, 1);
    EXPECT_EQ(p->y, 2);

    ecs.remove_component<Position>(e);
    EXPECT_FALSE(ecs.has_component<Position>(e));
  }
  TEST(ECS, get_after_remove_returns_null) {
    ECS ecs{100};

    auto e = ecs.create_entity();

    ecs.add_component<Position>(e, {1,2});
    ecs.remove_component<Position>(e);

    EXPECT_FALSE(ecs.has_component<Position>(e));
    EXPECT_EQ(ecs.get_component<Position>(e), nullptr);
  }
  TEST(ECS, destroying_entity_removes_components) {
    ECS ecs{100};

    auto e = ecs.create_entity();
    ecs.add_component<Position>(e, {5, 6});

    ecs.destroy_entity(e);

    EXPECT_FALSE(ecs.is_alive(e));
  }
  TEST(ECS, invalid_entity_access_is_safe) {
    ECS ecs{100};

    auto e = ecs.create_entity();
    ecs.destroy_entity(e);

    EXPECT_FALSE(ecs.is_alive(e));

    EXPECT_FALSE(ecs.has_component<Position>(e));

    auto* p = ecs.get_component<Position>(e);
    EXPECT_EQ(p, nullptr);
  }
  TEST(ECS, iterate_single_component) {
    ECS ecs{100};

    auto e1 = ecs.create_entity();
    auto e2 = ecs.create_entity();

    ecs.add_component<Position>(e1, {1, 1});
    ecs.add_component<Position>(e2, {2, 2});

    int count = 0;

    ecs.for_each_component<Position>([&](Entity, Position& p) {
        ++count;
        });

    EXPECT_EQ(count, 2);
  }
  struct Velocity { int dx, dy; };

  TEST(ECS, iterate_multiple_components) {
    ECS ecs{100};

    auto e1 = ecs.create_entity();
    auto e2 = ecs.create_entity();

    ecs.add_component<Position>(e1, {1, 1});
    ecs.add_component<Velocity>(e1, {1, 0});

    ecs.add_component<Position>(e2, {2, 2}); // no velocity

    int count = 0;

    ecs.for_each_components<Position, Velocity>(
        [&](Entity, Position&, Velocity&) {
        ++count;
        });

    EXPECT_EQ(count, 1); // only e1 qualifies
  }
  TEST(ECS, clear_resets_all_state) {
    ECS ecs{100};

    auto e = ecs.create_entity();
    ecs.add_component<Position>(e, {1,2});

    ecs.clear();

    EXPECT_FALSE(ecs.is_alive(e));

    auto e2 = ecs.create_entity();
    EXPECT_TRUE(ecs.is_alive(e2));
  }
  TEST(ECS, stress_create_destroy) {
    constexpr std::size_t max_entities = 100'000;
    ECS ecs{max_entities};

    std::vector<Entity> entities;
    entities.reserve(max_entities);

    // 1. Create entities
    for (std::size_t i = 0; i < max_entities; ++i)
      entities.push_back(ecs.create_entity());

    // 2. Destroy all
    for (auto e : entities)
      ecs.destroy_entity(e);

    // 3. Recreate and verify validity
    for (std::size_t i = 0; i < max_entities; ++i) {
      auto e = ecs.create_entity();
      EXPECT_TRUE(ecs.is_alive(e));
    }
  }
  TEST(ECS, stress_random_operations) {
    ECS ecs{1'000};
    std::vector<Entity> live;

    for (int i = 0; i < 1'000; i++)
    {
      int op = rand() % 3;

      if (op == 0 && live.size() < 1'000)
      {
        live.push_back(ecs.create_entity());
      }
      else if (op == 1 && !live.empty())
      {
        auto e = live.back();
        live.pop_back();
        ecs.destroy_entity(e);
      }
      else if (op == 2 && !live.empty())
      {
        auto e = live.back();
        ecs.add_component<Position>(e, {i, i});
      }
    }

    for (auto e : live)
      EXPECT_TRUE(ecs.is_alive(e));
  }

}

namespace math_tests {
  using namespace engine::math;
  TEST(MATH, vector_creation) {
    vec3 vector{};
    vec3 vector2{};
    EXPECT_TRUE(vector.x == vector2.x);
  }
  TEST(MATH, aabb_creation) {
    AABB aabb{};
    AABB aabb2{};
    EXPECT_TRUE(!aabb.valid() && !aabb2.valid());
    EXPECT_TRUE(aabb.min.x == aabb2.min.x);
  }
  TEST(MATH, aabb_test) {
    AABB aabb{};
    EXPECT_FALSE(aabb.valid());
  }

}

// import engine.core;
// namespace window_tests {
//   using namespace engine::platform;
//   using namespace engine::core;
//   TEST(WINDOW, window_creation) {
//     platform_context platform;
//     Window window(1920, 1080, "my window");
//     input_action_map action_map;
//     input_state state;
//     state.set_window(&window);
//     action_id close_window = action_map.create_action("close_window");
//     action_map.bind_key(close_window, button::backspace);
//     while(!window.should_close()) {
//       state.update();
//       if (action_map.is_action_pressed(close_window, state)) {
//         window.request_close();
//         std::cout << "closing window";
//       } else
//         logger::info("Window running");
//     }
//   }
//
// }
