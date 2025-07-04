#pragma once
#include <glm/glm.hpp>

struct AABB {
    glm::vec3 min;
    glm::vec3 max;

    // Default constructor — initializes to zero-size box
    AABB()
        : min(0.0f), max(0.0f) {}

    // Construct from min and max
    AABB(const glm::vec3 &min, const glm::vec3 &max)
        : min(min), max(max) {}

    // Construct from center and size
    static AABB fromCenterSize(const glm::vec3 &center, const glm::vec3 &size) {
        glm::vec3 halfSize = size * 0.5f;
        return AABB(center - halfSize, center + halfSize);
    }
    // Gets the farthest point from a given normal vector
    [[nodiscard]] glm::vec3 getFarthestPoint(const glm::vec3 &normal) const {
        return {
            (normal.x > 0) ? max.x : min.x,
            (normal.y > 0) ? max.y : min.y,
            (normal.z > 0) ? max.z : min.z};
    }
    // Get the size of the AABB
    [[nodiscard]] glm::vec3 size() const {
        return max - min;
    }

    // Get the center of the AABB
    [[nodiscard]] glm::vec3 center() const {
        return (min + max) * 0.5f;
    }

    // Check if this AABB intersects another AABB
    [[nodiscard]] static bool intersects(const AABB &a, const AABB &b) {
        return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
               (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
               (a.min.z <= b.max.z && a.max.z >= b.min.z);
    }
    [[nodiscard]] bool intersects(const AABB &other) const {
        return intersects(*this, other);
    }
    // Optional: check if point is inside
    [[nodiscard]] bool contains(const glm::vec3 &point) const {
        return (point.x >= min.x && point.x <= max.x) &&
               (point.y >= min.y && point.y <= max.y) &&
               (point.z >= min.z && point.z <= max.z);
    }
    void expandToInclude(const glm::vec3 &point) {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }

    void expandToInclude(const AABB &other) {
        expandToInclude(other.min);
        expandToInclude(other.max);
    }
};
