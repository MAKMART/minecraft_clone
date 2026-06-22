export module engine.math:aabb;

import std;
import engine.core;
import :types;

using engine::math::vec3;
export namespace engine::math {
  struct AABB final {
    AABB() noexcept
      : min(std::numeric_limits<f32>::infinity()),
      max(-std::numeric_limits<f32>::infinity())
    {}

    AABB(const vec3& min, const vec3& max) noexcept
      : min(min), max(max) {}

    [[nodiscard]] static AABB point(vec3 p) noexcept { return AABB(p, p); }

    [[nodiscard]] static AABB fromCenterSize(const vec3& center, const vec3& size) noexcept {
      vec3 halfSize = size * 0.5f;
      return AABB(center - halfSize, center + halfSize);
    }

    [[nodiscard]] static AABB fromCenterExtent(const vec3& center, const vec3& extent) noexcept {
      return AABB(center - extent, center + extent);
    }

    [[nodiscard]] constexpr vec3 getFarthestPoint(const vec3& normal) const noexcept {
      const vec3 mask = greaterThan(normal, vec3(0.0f));
      return engine::math::mix(min, max, mask);
    }

    [[nodiscard]] constexpr vec3 size() const noexcept { return max - min; }

    [[nodiscard]] constexpr vec3 center() const noexcept { return (min + max) * 0.5f; }

    [[nodiscard]] constexpr vec3 extent() const noexcept { return (max - min) * 0.5f; }

    [[nodiscard]] constexpr bool valid() const noexcept { return min.x <= max.x && min.y <= max.y && min.z <= max.z; }

    [[nodiscard]] static AABB unite(const AABB& a, const AABB& b) noexcept {
      return AABB(engine::math::min(a.min, b.min), engine::math::max(a.max, b.max));
    }

    [[nodiscard]] static AABB intersection(const AABB& a, const AABB& b) noexcept {
      return AABB(engine::math::max(a.min, b.min), engine::math::min(a.max, b.max));
    }

    [[nodiscard]] constexpr bool intersects(const AABB& other) const noexcept {
      return (min.x <= other.max.x && max.x >= other.min.x) &&
        (min.y <= other.max.y && max.y >= other.min.y) &&
        (min.z <= other.max.z && max.z >= other.min.z);
    }

    [[nodiscard]] constexpr bool contains(const vec3& point) const noexcept {
      return (point.x >= min.x && point.x <= max.x) &&
        (point.y >= min.y && point.y <= max.y) &&
        (point.z >= min.z && point.z <= max.z);
    }

    void expandToInclude(const vec3& point) noexcept {
      min = engine::math::min(min, point);
      max = engine::math::max(max, point);
    }

    void expandToInclude(const AABB& other) noexcept {
      min = engine::math::min(min, other.min);
      max = engine::math::max(max, other.max);
    }
    vec3 min;
    vec3 max;
  };
}
