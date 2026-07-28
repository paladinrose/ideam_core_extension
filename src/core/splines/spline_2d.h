#pragma once

#include "spline_base.h"
#include "polygon_math_2d.h"
#include <vector>

namespace ideam::core {

template <typename T = float>
class Spline2D : public SplineBase<2, T> {
public:
    using VecType = typename SplineBase<2, T>::VecType;

    Spline2D() = default;
    virtual ~Spline2D() = default;

    // --- 2D Spline Evaluation ---

    /**
     * Subdivides the spline into a dense array of flattened line segments[cite: 1].
     * Required for turning Bezier/Catmull curves into processable polygons.
     */
    [[nodiscard]] std::vector<VecType> get_smoothed_points(int p_max_subdivisions = 10) const;

    // --- Polygon Wrappers ---

    [[nodiscard]] bool contains_point(const VecType& p_point, int p_subdivisions = 10) const;
    [[nodiscard]] T get_area(int p_subdivisions = 10) const;
    [[nodiscard]] std::vector<uint32_t> triangulate(int p_subdivisions = 10) const;

    // --- CSG Booleans (Stubs for next phase) ---
    // These will eventually utilize the DOD Transient Memory Allocator 
    // to handle the complex intersection sequencing without heap fragmentation.
    
    //std::vector<Spline2D<T>> combine_polygons(const Spline2D<T>& p_other) const;
    //std::vector<Spline2D<T>> subtract_polygons(const Spline2D<T>& p_other) const;
    //std::vector<Spline2D<T>> intersect_polygons(const Spline2D<T>& p_other) const;

    // --- 2D Shape Generators ---

    [[nodiscard]] static Spline2D<T> create_rectangle(const VecType& p_center, const VecType& p_size);
    [[nodiscard]] static Spline2D<T> create_regular_polygon(int p_sides = 3, T p_radius = static_cast<T>(0.5), T p_angle_offset = static_cast<T>(0.0));
    [[nodiscard]] static Spline2D<T> create_star(int p_sides = 3, T p_outer_radius = static_cast<T>(0.5), T p_inner_radius = static_cast<T>(0.25), T p_angle_offset = static_cast<T>(0.0));
};

} // namespace ideam::core