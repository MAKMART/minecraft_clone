#pragma once
import glm;
#include <limits>

struct AABB {
	glm::vec3 min;
	glm::vec3 max;

	AABB() noexcept : min(std::numeric_limits<float>::infinity()), max(-std::numeric_limits<float>::infinity()) {}

	AABB(const glm::vec3& min, const glm::vec3& max) noexcept : min(min), max(max)
	{
	}

	static AABB fromCenterSize(const glm::vec3& center, const glm::vec3& size) noexcept
	{
		glm::vec3 halfSize = size * 0.5f;
		return AABB(center - halfSize, center + halfSize);
	}

	static AABB fromCenterExtent(const glm::vec3& center, const glm::vec3& extent) noexcept
	{
		return AABB(center - extent, center + extent);
	}

	[[nodiscard]] constexpr glm::vec3 getFarthestPoint(const glm::vec3& normal) const noexcept
	{
		const glm::bvec3 mask = glm::greaterThan(normal, glm::vec3(0.0f));
		return glm::mix(min, max, mask);
		/*
		return {
		    (normal.x > 0) ? max.x : min.x,
		    (normal.y > 0) ? max.y : min.y,
		    (normal.z > 0) ? max.z : min.z};
			*/
	}

	[[nodiscard]] constexpr glm::vec3 size() const noexcept { return max - min; }

	[[nodiscard]] constexpr glm::vec3 center() const noexcept { return (min + max) * 0.5f; }

	[[nodiscard]] constexpr glm::vec3 extent() const noexcept { return (max - min) * 0.5f; }

	[[nodiscard]] constexpr bool valid() const noexcept { return min.x <= max.x && min.y <= max.y && min.z <= max.z; }

	[[nodiscard]] static AABB unite(const AABB& a, const AABB& b) noexcept
	{
		return AABB(glm::min(a.min, b.min), glm::max(a.max, b.max));
	}

	[[nodiscard]] static AABB intersection(const AABB& a, const AABB& b) noexcept
	{
		return AABB(glm::max(a.min, b.min), glm::min(a.max, b.max));
	}

	[[nodiscard]] constexpr bool intersects(const AABB& other) const noexcept
	{
		return (min.x <= other.max.x && max.x >= other.min.x) &&
			(min.y <= other.max.y && max.y >= other.min.y) &&
			(min.z <= other.max.z && max.z >= other.min.z);
	}

	[[nodiscard]] constexpr bool contains(const glm::vec3& point) const noexcept
	{
		return (point.x >= min.x && point.x <= max.x) &&
		       (point.y >= min.y && point.y <= max.y) &&
		       (point.z >= min.z && point.z <= max.z);
	}

	void expandToInclude(const glm::vec3& point) noexcept
	{
		min = glm::min(min, point);
		max = glm::max(max, point);
	}

	void expandToInclude(const AABB& other) noexcept
	{
		min = glm::min(min, other.min);
		max = glm::max(max, other.max);
	}
};
