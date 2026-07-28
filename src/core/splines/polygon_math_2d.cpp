#include "polygon_math_2d.h"
#include <cmath>

namespace ideam::core{

template <typename T>
bool PolygonMath2D<T>::contains_point(std::span<const Vec2> p_polygon, const Vec2& p_point) noexcept {
    if (p_polygon.size() < 3) return false;

    bool inside = false;
    for (size_t i = 0, j = p_polygon.size() - 1; i < p_polygon.size(); j = i++) {
        const Vec2& pi = p_polygon[i];
        const Vec2& pj = p_polygon[j];

        if (((pi.y <= p_point.y && p_point.y < pj.y) || (pj.y <= p_point.y && p_point.y < pi.y)) &&
            (p_point.x < (pj.x - pi.x) * (p_point.y - pi.y) / (pj.y - pi.y != 0 ? pj.y - pi.y : static_cast<T>(1.0)) + pi.x)) {
            inside = !inside;
        }
    }
    return inside;
}

template <typename T>
T PolygonMath2D<T>::area(std::span<const Vec2> p_polygon) noexcept {
    if (p_polygon.size() < 3) return static_cast<T>(0.0);
    
    T a = static_cast<T>(0.0);
    for (size_t p = p_polygon.size() - 1, q = 0; q < p_polygon.size(); p = q++) {
        a += (p_polygon[p].x * p_polygon[q].y) - (p_polygon[q].x * p_polygon[p].y);
    }
    return a * static_cast<T>(0.5);
}

template <typename T>
bool PolygonMath2D<T>::line_intersection(const Vec2& p_a1, const Vec2& p_a2, const Vec2& p_b1, const Vec2& p_b2, Vec2& r_intersect) noexcept {
    Vec2 s1 = p_a2 - p_a1;
    Vec2 s2 = p_b2 - p_b1;

    T determinant = (-s2.x * s1.y + s1.x * s2.y);
    if (std::abs(determinant) < static_cast<T>(1e-6)) {
        return false; // Collinear or parallel
    }

    T s = (-s1.y * (p_a1.x - p_b1.x) + s1.x * (p_a1.y - p_b1.y)) / determinant;
    T t = ( s2.x * (p_a1.y - p_b1.y) - s2.y * (p_a1.x - p_b1.x)) / determinant;

    if (s >= static_cast<T>(0.0) && s <= static_cast<T>(1.0) && t >= static_cast<T>(0.0) && t <= static_cast<T>(1.0)) {
        r_intersect = p_a1 + (t * s1);
        return true;
    }
    return false;
}

template <typename T>
bool PolygonMath2D<T>::is_point_in_triangle(const Vec2& p_a, const Vec2& p_b, const Vec2& p_c, const Vec2& p_p) noexcept {
    T cx = p_p.x, cy = p_p.y;
    T ax = p_a.x, ay = p_a.y;
    T bx = p_b.x, by = p_b.y;
    T ccx = p_c.x, ccy = p_c.y;
    
    T w0 = (cx - ax) * (by - ay) - (cy - ay) * (bx - ax);
    T w1 = (cx - bx) * (ccy - by) - (cy - by) * (ccx - bx);
    T w2 = (cx - ccx) * (ay - ccy) - (cy - ccy) * (ax - ccx);
    
    return (w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0);
}

template <typename T>
bool PolygonMath2D<T>::snip(int u, int v, int w, int n, const std::vector<int>& V, std::span<const Vec2> p_polygon) noexcept {
    Vec2 a = p_polygon[V[u]];
    Vec2 b = p_polygon[V[v]];
    Vec2 c = p_polygon[V[w]];

    // Epsilon test for degenerate triangles
    if (1e-6f > (((b.x - a.x) * (c.y - a.y)) - ((b.y - a.y) * (c.x - a.x)))) {
        return false;
    }

    for (int p = 0; p < n; p++) {
        if ((p == u) || (p == v) || (p == w)) continue;
        if (is_point_in_triangle(a, b, c, p_polygon[V[p]])) return false;
    }
    return true;
}

template <typename T>
std::vector<uint32_t> PolygonMath2D<T>::triangulate(std::span<const Vec2> p_polygon) {
    std::vector<uint32_t> indices;
    int n = static_cast<int>(p_polygon.size());
    if (n < 3) return indices;

    // We often drop the closing vertex if it overlaps perfectly with index 0
    if (glm::distance(p_polygon.front(), p_polygon.back()) < static_cast<T>(1e-4)) {
        n -= 1;
    }

    std::vector<int> V(n);
    if (area(p_polygon.subspan(0, n)) > static_cast<T>(0.0)) {
        for (int v = 0; v < n; v++) V[v] = v;
    } else {
        for (int v = 0; v < n; v++) V[v] = (n - 1) - v;
    }

    int nv = n;
    int count = 2 * n;

    for (int v = nv - 1; nv > 2;) {
        if ((count--) <= 0) return indices; // Fallback to prevent infinite loop on complex self-intersections

        int u = v; if (nv <= u) u = 0;
        v = u + 1; if (nv <= v) v = 0;
        int w = v + 1; if (nv <= w) w = 0;

        if (snip(u, v, w, nv, V, p_polygon)) {
            indices.push_back(V[u]);
            indices.push_back(V[v]);
            indices.push_back(V[w]);

            for (int s = v, t = v + 1; t < nv; s++, t++) {
                V[s] = V[t];
            }
            nv--;
            count = 2 * nv;
        }
    }
    return indices;
}

// Explicit Instantiations
template struct PolygonMath2D<float>;
template struct PolygonMath2D<double>;

} // namespace ideam::core