#include <gtest/gtest.h>

import engine.math;

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
