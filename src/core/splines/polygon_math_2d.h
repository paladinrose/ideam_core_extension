#pragma once

#include <glm/glm.hpp>
#include <span>
#include <vector>

namespace ideam::core {

/**
 * PolygonMath2D
 * Pure, stateless geometric functions. 
 * Built to accept raw memory spans, allowing DOD views to bypass OOP overhead.
 */
template <typename T = float>
struct PolygonMath2D {
    using Vec2 = glm::vec<2, T>;

    /**
     * Ray-casting winding algorithm to check if a point is inside a polygon.
     */
    [[nodiscard]] static bool contains_point(std::span<const Vec2> p_polygon, const Vec2& p_point) noexcept;

    /**
     * Calculates the total surface area of a polygon using the shoelace formula.
     */
    [[nodiscard]] static T area(std::span<const Vec2> p_polygon) noexcept;

    /**
     * Ear-clipping triangulation. Returns a flat list of indices forming triangles.
     */
    [[nodiscard]] static std::vector<uint32_t> triangulate(std::span<const Vec2> p_polygon);

    /**
     * Calculates the intersection point between two line segments (A-B and C-D).
     * @return True if an intersection exists within the segment bounds, false otherwise.
     */
    [[nodiscard]] static bool line_intersection(const Vec2& p_a1, const Vec2& p_a2, const Vec2& p_b1, const Vec2& p_b2, Vec2& r_intersect) noexcept;

private:
    [[nodiscard]] static bool is_point_in_triangle(const Vec2& p_a, const Vec2& p_b, const Vec2& p_c, const Vec2& p_p) noexcept;
    [[nodiscard]] static bool snip(int u, int v, int w, int n, const std::vector<int>& V, std::span<const Vec2> p_polygon) noexcept;
};

} // namespace ideam::core