#include "spline_2d.h"
#include <cmath>

namespace ideam::core {

template <typename T>
std::vector<typename Spline2D<T>::VecType> Spline2D<T>::get_smoothed_points(int p_max_subdivisions) const {
    if (p_max_subdivisions < 3) p_max_subdivisions = 3;
    
    std::vector<VecType> smooth_points;
    if (this->points.empty()) return smooth_points;

    for (size_t i = 0; i < this->points.size() - 1; ++i) {
        smooth_points.push_back(this->points[i].point);

        const auto& sec = this->sections[i];
        if (sec.curve_type == CurveType::CATMULL_ROM || sec.curve_type == CurveType::CUBIC_BEZIER) {
            
            // Extract local percentages for this segment
            T pa = static_cast<T>(i) / static_cast<T>(this->points.size() - 1);
            T pb = static_cast<T>(i + 1) / static_cast<T>(this->points.size() - 1);
            
            T per_step = (pb - pa) / static_cast<T>(p_max_subdivisions);

            for (int j = 1; j < p_max_subdivisions; ++j) {
                smooth_points.push_back(this->get_point(pa + (per_step * static_cast<T>(j)), false));
            }
        }
    }
    smooth_points.push_back(this->points.back().point);
    return smooth_points;
}

// --- Polygon Wrappers ---

template <typename T>
bool Spline2D<T>::contains_point(const VecType& p_point, int p_subdivisions) const {
    if (!this->is_wrap) return false;
    std::vector<VecType> pts = get_smoothed_points(p_subdivisions);
    return PolygonMath2D<T>::contains_point(pts, p_point);
}

template <typename T>
T Spline2D<T>::get_area(int p_subdivisions) const {
    if (!this->is_wrap) return static_cast<T>(0.0);
    std::vector<VecType> pts = get_smoothed_points(p_subdivisions);
    return PolygonMath2D<T>::area(pts);
}

template <typename T>
std::vector<uint32_t> Spline2D<T>::triangulate(int p_subdivisions) const {
    if (!this->is_wrap) return std::vector<uint32_t>();
    std::vector<VecType> pts = get_smoothed_points(p_subdivisions);
    return PolygonMath2D<T>::triangulate(pts);
}

// --- Shape Generators ---

template <typename T>
Spline2D<T> Spline2D<T>::create_rectangle(const VecType& p_center, const VecType& p_size) {
    Spline2D<T> rect;
    T half_x = p_size.x * static_cast<T>(0.5);
    T half_y = p_size.y * static_cast<T>(0.5);

    rect.add_point(VecType(p_center.x - half_x, p_center.y - half_y));
    rect.add_point(VecType(p_center.x - half_x, p_center.y + half_y));
    rect.add_point(VecType(p_center.x + half_x, p_center.y + half_y));
    rect.add_point(VecType(p_center.x + half_x, p_center.y - half_y));
    
    rect.set_wrap(true);
    return rect;
}

template <typename T>
Spline2D<T> Spline2D<T>::create_regular_polygon(int p_sides, T p_radius, T p_angle_offset) {
    Spline2D<T> poly;
    if (p_sides < 3) p_sides = 3;

    const T pi = static_cast<T>(3.14159265358979323846);
    T angle_step = (pi * static_cast<T>(2.0)) / static_cast<T>(p_sides);
    
    // Original C# reversed the polygon creation, so we loop backwards[cite: 1]
    for (int i = p_sides; i > 0; --i) {
        T current_angle = (angle_step * static_cast<T>(i)) + p_angle_offset;
        poly.add_point(VecType(p_radius * std::cos(current_angle), p_radius * std::sin(current_angle)));
    }
    poly.set_wrap(true);
    return poly;
}

template <typename T>
Spline2D<T> Spline2D<T>::create_star(int p_sides, T p_outer_radius, T p_inner_radius, T p_angle_offset) {
    Spline2D<T> star;
    if (p_sides < 3) p_sides = 3;
    
    int total_points = p_sides * 2;
    const T pi = static_cast<T>(3.14159265358979323846);
    T angle_step = (pi * static_cast<T>(2.0)) / static_cast<T>(total_points);
    
    bool inner = false;
    for (int i = 1; i <= total_points; ++i) {
        T current_angle = (angle_step * static_cast<T>(i)) + p_angle_offset;
        T current_radius = inner ? p_inner_radius : p_outer_radius;
        
        star.add_point(VecType(current_radius * std::cos(current_angle), current_radius * std::sin(current_angle)));
        inner = !inner;
    }
    star.set_wrap(true);
    return star;
}

// Explicit Instantiations
template class Spline2D<float>;
template class Spline2D<double>;

} // namespace ideam::core